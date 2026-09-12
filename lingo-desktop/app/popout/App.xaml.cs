using System.Drawing;
using System.Windows;
using Forms = System.Windows.Forms;

namespace LingoUi;

public partial class App : System.Windows.Application
{
    private Forms.NotifyIcon? tray;

    protected override void OnStartup(StartupEventArgs args)
    {
        base.OnStartup(args);
        tray = new Forms.NotifyIcon { Icon = SystemIcons.Application, Text = "Lingo", Visible = true };
        var menu = new Forms.ContextMenuStrip();
        menu.Items.Add("Show Popout", null, (_, _) => Current.MainWindow?.Show());
        menu.Items.Add("Exit", null, (_, _) => Shutdown());
        tray.ContextMenuStrip = menu;
    }

    protected override void OnExit(ExitEventArgs args)
    {
        if (tray != null) { tray.Visible = false; tray.Dispose(); }
        base.OnExit(args);
    }
}
