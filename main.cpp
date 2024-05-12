#include <SFML/Audio.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "notif.cpp"

using namespace cv;
using namespace std;

const int COLOR_REDUCE = -1;

const int MODE_COLOR = 0;
const int MODE_MONOCHROME = 1;
const int MODE_256 = 2;
int COLOR_MODE = 0;

sf::Music audioBuffer;

void onExit(int s) {
	// Reset terminal colors and formatting
	cout << "\033[0m\033[H\033[J\033[?25h" << endl;

	// Make sure to get rid of the temporary file from audio processing
	std::remove("temp.wav");
	std::remove("temp");

	// Stop the audio
	audioBuffer.stop();

	// Clear the onExit signal to prevent possible recursion
	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = NULL;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);

	exit(1);
}

inline int coordsToScreenBufferIndex(int i, int j, int screenWidth) {
	return (i * screenWidth + j) * 3;
}

inline int similarityBetweenPixels(Vec3b vec1, Vec3b vec2) {
	return abs(vec1[0] - vec2[0]) + abs(vec1[1] - vec2[1]) + abs(vec1[2] - vec2[2]);
}


int main(int argc, char *argv[]) {
	// Setup the onExit signal to properly close the program
	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = onExit;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);

	// Get the size of the terminal
	struct winsize terminalSize;
	ioctl(0, TIOCGWINSZ, &terminalSize);

	// Disable warning messages from opencv that mess up video
	setenv("OPENCV_LOG_LEVEL", "OFF", 1);
	setenv("OPENCV_FFMPEG_LOGLEVEL", "-8", 1);

	long startOffset = 0;
	float volume = 100;

	// Enforce arguments
	if (argc < 2) {
		cout << "Usage: " << argv[0] << " <video_name> [arguments]" << endl;
		cout << "Run with --help for more information" << endl;
		exit(1);
	}

	// Help message
	if (!std::string("--help").compare(argv[1])) {
		cout << "TerinalVideo2" << endl;
		cout << "Usage: " << argv[0] << " <video_name> [arguments]" << endl << endl;
		cout << "Arguments: " << endl;
		cout << " --color-mode [mode]  -c [mode]     Set the color mode: m monochrome, c color, 256 256-compatability" << endl;
		cout << " --help               -h            Display this help screen" << endl;
		cout << " --offset [ms]        -o [ms]       Start [ms] milliseconds into the video" << endl;
		cout << " --volume             -v [percent]  Set the volume in range 0% to 100%" << endl << endl;
		cout << "Controls: " << endl;
		cout << " Left and right arrow keys			 Skip 5 seconds backward or forward respectively" << endl;
		cout << " Up and down arrow keys			 Raise and lower the volume by 10% respectively" << endl;
		exit(0);
	}

	// If arguments were provided
	if (argc > 2) {
		int argIndex = 2;
		while (argIndex < argc) {
			// For --offset
			if (!std::string("-o").compare(argv[argIndex]) || !std::string("--offset").compare(argv[argIndex])) {
				startOffset = stoi(string(argv[argIndex + 1]));
				
				argIndex++; // Make sure to increment one extra to skip the number
			} else if (!std::string("-v").compare(argv[argIndex]) || !std::string("--volume").compare(argv[argIndex])) {
				volume = stof(string(argv[argIndex + 1]));
				
				argIndex++; // Make sure to increment one extra to skip the number
			} else if (!std::string("-c").compare(argv[argIndex]) || !std::string("--color-mode").compare(argv[argIndex])) {
				if (argv[argIndex + 1][0] == ("c")[0]) {
					COLOR_MODE = MODE_COLOR;
				} else if (argv[argIndex + 1][0] == ("m")[0]) {
					COLOR_MODE = MODE_MONOCHROME;
				} else if (argv[argIndex + 1][0] == ("2")[0]) {
					COLOR_MODE = MODE_256;
				}

				argIndex++; // Make sure to increment one extra to skip the mode
			} else {
				cout << "Invalid argument: " << argv[argIndex] << endl;
				exit(0);
			}

			argIndex++;
		}
	}

	// Attempt to load the video file into opencv
	cv::VideoCapture capture;
	cv::Mat RGB;

	string videoPath = argv[1];

	if(!capture.open(videoPath))
	{
		cout<<"Video Not Found"<<endl;
		return 1;
	}


	// Use ffmpeg to extract the audio from the video
	cout << "Getting audio..." << endl;

	std::ostringstream command;
	command << "ffmpeg -v error -stats -i ";
	command << videoPath;
	command << " -vn -f wav ./temp.wav -y";
	system(command.str().c_str());

	rename("./temp.wav", "./temp"); // Rename the file to make it seem even more temporary
	
	audioBuffer.openFromFile("./temp");
	audioBuffer.setPlayingOffset(sf::milliseconds(startOffset)); // Make sure to compensate for the offset
	audioBuffer.setVolume(volume);
	audioBuffer.play();

	const int FPS = capture.get(cv::CAP_PROP_FPS);

	// Get the current time (minus start offset)
	auto time = std::chrono::system_clock::now();
	auto since_epoch = time.time_since_epoch();
	auto originalMillis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
	long start = originalMillis.count() - startOffset;

	// Go down a bunch of lines to prevent the video from overwriting what's already in terminal
	for (int i = 0; i < terminalSize.ws_row; ++i) {
		cout << endl;
	}

	bool wasLeft = false;
	bool wasRight = false;
	bool wasUp = false;
	bool wasDown = false;

	// The screen buffer holds the previous frame and is checked against to prevent updating pixels that look the same between frames
	uint8_t screenBuffer[terminalSize.ws_row * terminalSize.ws_col * 3];
	bool screenBufferInited = false;

	while (true) {
		// Get the current time
		auto time = std::chrono::system_clock::now();
		auto since_epoch = time.time_since_epoch();
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);

		// Skipping 5 seconds logic
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && !wasLeft) {
			wasLeft = true;
			startOffset -= 5000;
			if (millis.count() - (originalMillis.count() - startOffset) < 0) {
				startOffset -= millis.count() - (originalMillis.count() - startOffset);
			} 
			start = originalMillis.count() - startOffset;
			audioBuffer.setPlayingOffset(sf::milliseconds(startOffset + (millis.count() - originalMillis.count())));
			
			addNotification(new Notification("Skipped 5 seconds back"));
		} else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && !wasRight) {
			wasRight = true;
			startOffset += 5000;
			start = originalMillis.count() - startOffset;
			audioBuffer.setPlayingOffset(sf::milliseconds(startOffset + (millis.count() - originalMillis.count())));
			
			addNotification(new Notification("Skipped 5 seconds forward"));
		}

		if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && wasLeft)	wasLeft = false;
		if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && wasRight)	wasRight = false;

		// Changing volume logic
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && !wasUp) {
			wasUp = true;
			volume += 10;
			if (volume > 100) volume = 100;
			audioBuffer.setVolume(volume);
			
			addNotification(new Notification("Volume raised to " + to_string((int) volume) + "%"));
		} else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && !wasDown) {
			wasDown = true;
			volume -= 10;
			if (volume < 0) volume = 0;
			audioBuffer.setVolume(volume);
			
			addNotification(new Notification("Volume lowered to " + to_string((int) volume) + "%"));
		}

		if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && wasUp)	wasUp = false;
		if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && wasDown)	wasDown = false;

		// Read a specific frame from the video using the current time
		capture.set(cv::CAP_PROP_POS_MSEC, millis.count() - start);
		capture >> RGB;
		
		if (RGB.empty()) { // Check if video is over
			onExit(0); //cout << "Capture Finished" << endl;
		}

		// Setup obtaining pixel data
		uint8_t* pixelPtr = (uint8_t*)RGB.data;
		int cn = RGB.channels();
		Scalar_<uint8_t> bgrPixel;

		// The video needs to be scaled to fit the terminal
		float xScale = (float) RGB.cols / terminalSize.ws_col;
		float yScale = (float) RGB.rows / terminalSize.ws_row;

		// Make sure the monochrome colors are correct if possible
		if (COLOR_MODE == MODE_MONOCHROME) {
			cout << "\033[37;40m";
		}
		cout << "\033[H\033[?25l"; // Sets cursor position to the top-left-most position and makes the cursor not blink for betting looking text rendering

		int lastJ = -1;

		// For every character in the terminal
		for (int i = 0; i < terminalSize.ws_row; ++i) {
			for (int j = 0; j < terminalSize.ws_col; ++j) {
				// If notifications are rendered, don't overwrite them
				if (i < 8) {
					if (notificationsArr[i]) {
						if (j < (notificationsArr[i]->text).length()) {
							cout << "\033[1C";
							continue;
						}
					}
				}
				
				// Logic to prevent redrawing pixels that look the same between frames
				// This is mostly useful for videos with borders of some sort (i.e. movies or music videos)
				// Make sure not to do this to the first 8 lines to allow the notifications to be hidden
				Vec3b pixelBottom = RGB.at<Vec3b>(
					(int) (i * yScale) + ((int) yScale/2),
					(int) (j * xScale)
				);

				int index = coordsToScreenBufferIndex(i, j, terminalSize.ws_row);
				if (screenBufferInited) {
					if (i > 8 && abs(screenBuffer[index + 0] - pixelBottom[0]) + abs(screenBuffer[index + 1] - pixelBottom[1]) + abs(screenBuffer[index + 2] - pixelBottom[2]) < 1) {
						//cout << "\033[1C";
						continue;
					}
				}
				screenBuffer[index + 0] = pixelBottom[0];
				screenBuffer[index + 1] = pixelBottom[1];
				screenBuffer[index + 2] = pixelBottom[2];

				if (lastJ != j - 1) { // If the last character printed wasn't the previous one
					cout << "\033[" << (j - lastJ) - 1 << "C";
				}

				if (COLOR_MODE == MODE_COLOR) {
					// Obtain two pixels
					Vec3b pixelTop = RGB.at<Vec3b>(
						(int) (i * yScale),
						(int) (j * xScale)
					);

					// If color reduction is enabled, process that
					if (COLOR_REDUCE > 0) {
						float dither = (float) ((i + j) % 2) / 2.1;
						pixelTop[0] = roundf(pixelTop[0] / COLOR_REDUCE + dither) * COLOR_REDUCE;
						pixelTop[1] = roundf(pixelTop[1] / COLOR_REDUCE + dither) * COLOR_REDUCE;
						pixelTop[2] = roundf(pixelTop[2] / COLOR_REDUCE + dither) * COLOR_REDUCE;
						pixelBottom[0] = roundf(pixelBottom[0] / COLOR_REDUCE - dither) * COLOR_REDUCE;
						pixelBottom[1] = roundf(pixelBottom[1] / COLOR_REDUCE - dither) * COLOR_REDUCE;
						pixelBottom[2] = roundf(pixelBottom[2] / COLOR_REDUCE - dither) * COLOR_REDUCE;
					}

					// Set the background color to the top pixel, and the foreground color to the bottom pixel and print a half-block character
					// This gives the illusion of having double vertical resolution, since a block character is usually 1:1 and a character 1:2
					if (similarityBetweenPixels(pixelTop, pixelBottom) == 0) { // If the top and bottom pixels are the same, don't change both the background and foreground color
						cout << "\033[48;2;" << ((int) pixelTop[2]) << ";" << ((int) pixelTop[1]) << ";" << ((int) pixelTop[0]) << "m ";
					} else {
						cout << "\033[48;2;" << ((int) pixelTop[2]) << ";" << ((int) pixelTop[1]) << ";" << ((int) pixelTop[0]) << "m" << "\033[38;2;" << ((int) pixelBottom[2]) << ";" << ((int) pixelBottom[1]) << ";" << ((int) pixelBottom[0]) << "m▄";
					}
				} else if (COLOR_MODE == MODE_MONOCHROME) {
					Vec3b pixel = RGB.at<Vec3b>(
						(int) (i * yScale),
						(int) (j * xScale)
					);

					Vec3b pixelUp = Vec3b(255, 255, 255);
					if (i != 0) {
						pixelUp = RGB.at<Vec3b>(
							(int) (i * yScale) - ((int) yScale/2),
							(int) (j * xScale)
						);
					}

					Vec3b pixelDown = Vec3b(255, 255, 255);
					if (i + 1 != terminalSize.ws_row) {
						Vec3b pixelDown = RGB.at<Vec3b>(
							(int) (i * yScale) + ((int) yScale/2),
							(int) (j * xScale)
						);
					}

					// By mixing the colors like this, it closer mimics the grayscale color human eyes see, created a better looking grayscale
					float grayScale = ((0.2125 * pixel[0]) + (0.7154 * pixel[1]) + (0.0721 * pixel[2]));
					float grayScaleUp = ((0.2125 * pixelUp[0]) + (0.7154 * pixelUp[1]) + (0.0721 * pixelUp[2]));
					float grayScaleDown = ((0.2125 * pixelDown[0]) + (0.7154 * pixelDown[1]) + (0.0721 * pixelDown[2]));

					if (grayScale > 240 && grayScaleUp < 16) {
						cout << "▄";
						continue;
					} else if (grayScale > 240 && grayScaleDown < 16) {
						cout << "▀";
						continue;
					}

					// Normalize the values 0-5 and dither
					grayScale /= 25.6;
					if (((int) round(grayScale)) % 2 == 1) {
						if ((i + j) % 2 == 0 && grayScale - 0.4 > round(grayScale)) {
							grayScale = round(grayScale - 1);
						} else {
							grayScale = round(grayScale + 1);
						}
					}

					grayScale /= 2;

					// Convert a value to a character of a certain brightness
					string character;
					int grayScaleInt = (int) grayScale;
					if (grayScaleInt == 0) {
						character = " ";
					} else if (grayScaleInt == 1) {
						character = ".";
					} else if (grayScaleInt == 2) {
						character = "░";
					} else if (grayScaleInt == 3) {
						character = "▒";
					} else if (grayScaleInt == 4) {
						character = "▓";
					} else {
						character = "█";
					}

					cout << character;

					// (0.2125 * color.r) + (0.7154 * color.g) + (0.0721 * color.b)
				} else if (COLOR_MODE == MODE_256) {
					// Obtain two pixels
					Vec3b pixelTop = RGB.at<Vec3b>(
						(int) (i * yScale),
						(int) (j * xScale)
					);

					// Normalize colors 0-5
					pixelTop[0] = (j % 2 == 1) ? round(pixelTop[0] / 51) : floor(pixelTop[0] / 51);
					pixelTop[1] = (j % 2 == 1) ? round(pixelTop[1] / 51) : floor(pixelTop[1] / 51);
					pixelTop[2] = (j % 2 == 1) ? round(pixelTop[2] / 51) : floor(pixelTop[2] / 51);
					pixelBottom[0] = (j % 2 == 0) ? round(pixelBottom[0] / 51) : floor(pixelBottom[0] / 51);
					pixelBottom[1] = (j % 2 == 0) ? round(pixelBottom[1] / 51) : floor(pixelBottom[1] / 51);
					pixelBottom[2] = (j % 2 == 0) ? round(pixelBottom[2] / 51) : floor(pixelBottom[2] / 51);

					int topColor;
					int bottomColor;

					// The 256 color palette has extra shades of gray. This code uses that.
					if ((int) pixelTop[0] == (int) pixelTop[1] && (int) pixelTop[1] == (int) pixelTop[2]) {
						topColor = round(pixelTop[0] * 4.6) + 232;
					} else {
						topColor = pixelTop[2] + (pixelTop[1] * 6) + (pixelTop[0] * 36) + 16;
					}
					if ((int) pixelBottom[0] == (int) pixelBottom[1] && (int) pixelBottom[1] == (int) pixelBottom[2]) {
						bottomColor = round(pixelBottom[0] * 4.6) + 232;
					} else {
						bottomColor = pixelBottom[2] + (pixelBottom[1] * 6) + (pixelBottom[0] * 36) + 16;
					}

					// Set the background color to the top pixel, and the foreground color to the bottom pixel and print a half-block character
					// This gives the illusion of having double vertical resolution, since a block character is usually 1:1 and a character 1:2
					cout << "\033[48;5;" << topColor << "m\033[38;5;" << bottomColor << "m▄";
				}

				// Remember where the last character was printed
				lastJ = j;
			}

			// If this isn't the last row, go to the next line and reset lastJ
			if (i + 1 != terminalSize.ws_row) {
				cout << endl;
				lastJ = -1;
			}
		}

		// Reset the color
		cout << "\033[0m";

		// Since the screen has been drawn, the screenBuffer must be initialized by now
		screenBufferInited = true;

		// Process and draw notifications
		updateNotifications(2, 1000/FPS);


		// Wait until one frame's worth of time has passed, measured from when the current frame began
		while (true) {
			auto timeEndOfFrame = std::chrono::system_clock::now();
			auto since_epochEndOfFrame = timeEndOfFrame.time_since_epoch();
		
			if (std::chrono::duration_cast<std::chrono::milliseconds>(since_epochEndOfFrame).count() >= millis.count() + 1000/FPS) {
				break;
			}
		}
	}

	// Reset the color, close the opencv capture and the audio buffer
	cout << "\033[0" << endl;
	capture.release();
	audioBuffer.stop();
	return 0;
}