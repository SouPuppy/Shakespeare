using System.IO;
using System.IO.Pipes;
using System.Text;
using System.Windows;

namespace LingoUi;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        _ = Listen();
    }

    private async Task Listen()
    {
        try
        {
            using var pipe = new NamedPipeClientStream(".", "lingo", PipeDirection.In);
            await pipe.ConnectAsync(3000);
            using var reader = new StreamReader(pipe, Encoding.UTF8);
            Status.Text = "Connected";
            while (await reader.ReadLineAsync() is { } line)
            {
                if (!line.Contains("\"sentence\"")) continue;
                const string marker = "\"text\":\"";
                var start = line.IndexOf(marker, StringComparison.Ordinal) + marker.Length;
                var end = line.LastIndexOf('"');
                if (start >= marker.Length && end > start) Sentence.Text = "Focus: " + line[start..end];
            }
        }
        catch
        {
            Status.Text = "Waiting...";
        }
    }
}
