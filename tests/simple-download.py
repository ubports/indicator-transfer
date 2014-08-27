#!/usr/bin/python3
# source: https://wiki.ubuntu.com/DownloadService/DownloadManager#Code_Examples

from gi.repository import GLib
import dbus
from dbus.mainloop.glib import DBusGMainLoop

DBusGMainLoop(set_as_default=True)

MANAGER_PATH = '/'
MANAGER_IFACE = 'com.canonical.applications.DownloadManager'
DOWNLOAD_IFACE = 'com.canonical.applications.Download'
DOWNLOAD_URIS = [ 'http://i.imgur.com/y51njgu.jpg',
                  'http://upload.wikimedia.org/wikipedia/commons/c/c6/Bayerischer_Wald_-_Aufichtenwald_001.jpg',
                  'http://upload.wikimedia.org/wikipedia/commons/e/ea/Sydney_Harbour_Bridge_night.jpg' ]


def download_created(path):
    """Deal with the download created signal."""
    print('Download created in %s' % path)

def finished_callback(path, loop):
    global n_remaining
    """Deal with the finish signal."""
    print('Download performed in "%s"' % path)
    n_remaining -= 1
    if n_remaining == 0:
        loop.quit()


def progress_callback(total, progress):
    """Deal with the progress signals."""
    print('Progress is %s/%s' % (progress, total))

if __name__ == '__main__':
    global n_remaining
    n_remaining = 0

    bus = dbus.SessionBus()
    loop = GLib.MainLoop()
    manager = bus.get_object('com.canonical.applications.Downloader',
            MANAGER_PATH)
    manager_dev_iface = dbus.Interface(manager, dbus_interface=MANAGER_IFACE)

    # ensure that download created works
    manager_dev_iface.connect_to_signal('downloadCreated', download_created)

    for uri in DOWNLOAD_URIS:
        n_remaining += 1
        print('Adding "%s"' % uri)
        down_path = manager_dev_iface.createDownload((uri, "", "",
            dbus.Dictionary({}, signature="sv"),
            dbus.Dictionary({}, signature="ss")))
        download = bus.get_object('com.canonical.applications.Downloader', down_path)
        download_dev_iface = dbus.Interface(download, dbus_interface=DOWNLOAD_IFACE)
        download_dev_iface.connect_to_signal('progress', progress_callback)
        download_dev_iface.connect_to_signal('finished', lambda path: finished_callback(path, loop))
        download_dev_iface.start()

    loop.run()

