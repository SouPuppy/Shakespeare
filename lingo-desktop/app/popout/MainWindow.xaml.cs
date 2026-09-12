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
        Log("listener started");
        while (!stop.IsCancellationRequested)
        {
            try
            {
                using var pipe = new NamedPipeClientStream(".", "lingo-focus", PipeDirection.In);
                Log("connecting");
                await pipe.ConnectAsync(1000, stop.Token);
                Log("connected");
                using var reader = new StreamReader(pipe, Encoding.UTF8);
                SetStatus("Connected");
                while (!stop.IsCancellationRequested && await reader.ReadLineAsync(stop.Token) is { } line)
                {
                    Log($"received {line}");
                    HandleMessage(line);
                }
                Log("connection closed");
            }
            catch (OperationCanceledException) { return; }
            catch (Exception error) { Log(error.ToString()); SetStatus("Waiting..."); await Task.Delay(250, stop.Token); }
        }
    }

    private static void Log(string message) => File.AppendAllText(Path.Combine(AppContext.BaseDirectory, "lingo-popout.log"), $"{DateTime.Now:O} {message}\n");

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
        catch (JsonException error) { Log($"invalid json: {error.Message}; line={line}"); }
    }

    private void SetStatus(string value) => Dispatcher.Invoke(() => Status.Text = value);

    protected override void OnClosed(EventArgs e)
    {
        stop.Cancel();
        base.OnClosed(e);
    }
}
