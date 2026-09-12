using System.Drawing;
using System.Diagnostics;
using System.IO;
using System.Windows;
using Forms = System.Windows.Forms;

namespace LingoUi;

public partial class App : System.Windows.Application
{
    private Forms.NotifyIcon? tray;
    private Process? service;
    private Process? editor;

    protected override void OnStartup(StartupEventArgs args)
    {
        base.OnStartup(args);
        var bin = FindBinaryDirectory();
        service = Process.Start(new ProcessStartInfo(Path.Combine(bin, "lingo-service.exe")) { WorkingDirectory = bin, UseShellExecute = false, CreateNoWindow = true });
        editor = Process.Start(new ProcessStartInfo(Path.Combine(bin, "lingo-editor.exe")) { WorkingDirectory = bin, UseShellExecute = false });
        tray = new Forms.NotifyIcon { Icon = SystemIcons.Application, Text = "Lingo", Visible = true };
        var menu = new Forms.ContextMenuStrip();
        menu.Items.Add("Show Popout", null, (_, _) => Current.MainWindow?.Show());
        menu.Items.Add("Exit", null, (_, _) => Shutdown());
        tray.ContextMenuStrip = menu;
    }

    protected override void OnExit(ExitEventArgs args)
    {
        StopChild(editor);
        StopChild(service);
        if (tray != null) { tray.Visible = false; tray.Dispose(); }
        base.OnExit(args);
    }

    private static void StopChild(Process? process) { if (process == null || process.HasExited) return; process.Kill(true); process.Dispose(); }

    private static string FindBinaryDirectory()
    {
        var directory = new DirectoryInfo(AppContext.BaseDirectory);
        while (directory != null)
        {
            var candidate = Path.Combine(directory.FullName, "build", "bin", "Debug");
            if (File.Exists(Path.Combine(candidate, "lingo-service.exe"))) return candidate;
            directory = directory.Parent;
        }
        throw new FileNotFoundException("Cannot find lingo-service.exe");
    }
}
