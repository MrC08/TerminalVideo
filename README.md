# TerminalVideo
📺 🔊 A lightweight video player with sound for your terminal!

⚠️ Important Note: Requires ffmpeg to be installed on the system to work!⚠️

## Features 🛠️

 - Plays videos in all modern terminals and consoles
	 - Everything that supports ASCII characters and ANSI codes (basically every terminal since the 90s) will work!
 - Plays audio from the video using ffmpeg
 - Volume controls on the fly with the keyboard or through a command line argument
 - Skipping through the video in 5 second increments with the keyboard.-
 - Alternate color modes for terminals that don't support the normal set of ANSI color codes

## Color Modes 🎨
To access these color modes, use the `-c [mode]` or `--color-mode [mode]` arguments
 - Full Color (`c` or `color`)
	 - Uses the full spectrum of RGB color. This mode is the least supported; but when is it, it's beautiful. This mode offers the full range of color a video can have.
 - 256 Color (`256` or `256-compatibility`)
	 - Uses a more compatible palette of 256 colors. This mode look the worst, but it can allow terminals that do not otherwise support RGB color to still show some color.
 - Monochrome (`m` or `monochrome`)
	 - This mode renders the video in a way which resembles ASCII art. This mode is supported on every major terminal or console out there, as it only uses basic Unicode characters to get the job done.
 - ASCII Art (`a` or `ascii-art`)
	 - Makes the video look like ASCII art. This mode is extremely compatible, and should work on any terminal or console.

## Arguments ⚙️

 - `-c [mode]` or `--color-mode [mode]`
	 - Changes the color mode (see the *Color Modes* section).
 - `-h` or `-help`
	 - Displays the help text
 - `-o [ms]` or `-offset [ms]`
	 - Starts the video a specified number of milliseconds in.
 - `-v [percent]` or `--volume [percent]`
	 - Sets the volume to a percent. Make sure not to include the percent sign!

## Controls ⌨️

 - Left and right arrow keys
	 - Skips 5 seconds backward or forward in the video respectively.
 - Up and down arrow keys
	 - Raise and lower the volume by 10% respectively.
