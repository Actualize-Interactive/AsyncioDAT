# AsyncioDAT callbacks — worked example
#
# AsyncioDAT loads this module (by default "asyncio_dat_callbacks.py" beside the
# .toe, or the path set in config.toml [asyncio].callback_module_path) and calls
# the lifecycle functions below. on_start runs once, when the first AsyncioDAT
# instance is created.
#
# This example is intentionally dependency-free: it registers a small plugin and
# schedules a demo coroutine so you can see the lifecycle and plugin API working.

import asyncio


class HeartbeatPlugin:
    """A tiny example plugin stored on the AsyncioDAT instance.

    Plugins are arbitrary Python objects you register with set_plugin(); they
    persist on the operator and are reachable via op('Asyncio1').plugins.<name>.
    """

    def __init__(self):
        self.ticks = 0

    async def run(self, interval=1.0, count=5):
        for _ in range(count):
            await asyncio.sleep(interval)
            self.ticks += 1
            print(f"[heartbeat] tick {self.ticks}")
        print("[heartbeat] done")


def on_start(asyncio_dat):
    """Called once when the first AsyncioDAT instance is created.

    Note: this is not called from TouchDesigner's callbacks DAT and runs before
    the network is fully usable, so use it to set up libraries/services rather
    than to read or write operators. It's ideal for registering plugins or
    starting background services that act on the project once it's running.
    """
    print("on_start called by asyncio_dat:", asyncio_dat)

    heartbeat = HeartbeatPlugin()
    asyncio_dat.set_plugin("heartbeat", heartbeat)
    asyncio_dat.add_task(heartbeat.run())


def on_initialized(asyncio_dat):
    """Called after the event loop is (re)initialized.

    Not called for the very first instance on TouchDesigner startup — use it to
    re-register plugins/services after the operator is deactivated and
    reactivated. set_plugin overwrites, so it is safe to call again.
    """
    asyncio_dat.set_plugin("heartbeat", HeartbeatPlugin())


def on_poll_begin(asyncio_dat):
    """Called at the start of each poll (only when 'on_poll_begin' is enabled)."""
    pass


def on_poll_end(asyncio_dat):
    """Called at the end of each poll (only when 'on_poll_end' is enabled)."""
    pass


def on_shutdown_begin(asyncio_dat):
    """Called before plugins are cleared and the loop is stopped.

    Useful for cleaning up resources or stopping services started in on_start.
    """
    pass


def on_shutdown_complete(asyncio_dat):
    """Called after all plugins are cleared and the loop is stopped."""
    pass
