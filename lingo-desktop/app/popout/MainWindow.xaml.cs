using System.IO;
using System.IO.Pipes;
using System.Text;
using System.Text.Json;
using System.Windows;

namespace LingoUi;

public partial class MainWindow : Window
{
    private CancellationTokenSource stop = new();

    public MainWindow()
    {
        InitializeComponent();
        _ = ListenUntilClosed();
    }

    private async Task ListenUntilClosed()
    {
        while (!stop.IsCancellationRequested)
        {
            try
            {
                using var pipe = new NamedPipeClientStream(".", "lingo", PipeDirection.In);
                await pipe.ConnectAsync(1000, stop.Token);
                using var reader = new StreamReader(pipe, Encoding.UTF8);
                SetStatus("Connected");
                while (!stop.IsCancellationRequested && await reader.ReadLineAsync(stop.Token) is { } line) HandleMessage(line);
            }
            catch (OperationCanceledException) { return; }
            catch { SetStatus("Waiting..."); await Task.Delay(250, stop.Token); }
        }
    }

    private void HandleMessage(string line)
    {
        try
        {
            using var message = JsonDocument.Parse(line);
            var root = message.RootElement;
            if (root.TryGetProperty("type", out var type) && type.GetString() == "focus")
            {
                var sentence = root.TryGetProperty("sentence", out var sentenceValue) ? sentenceValue.GetString() ?? "" : "";
                var word = root.TryGetProperty("word", out var wordValue) ? wordValue.GetString() ?? "" : "";
                Dispatcher.Invoke(() => { Sentence.Text = sentence; Word.Text = word; });
            }
        }
        catch (JsonException) { }
    }

    private void SetStatus(string value) => Dispatcher.Invoke(() => Status.Text = value);

    protected override void OnClosed(EventArgs e)
    {
        stop.Cancel();
        base.OnClosed(e);
    }
}
