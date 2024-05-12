#include <SFML/Audio.hpp>
#include <stdio.h>
#include <iostream>

using namespace std;

class Notification {
	public:
		string text;
		int msLeft;

		Notification(std::string text) {
			this->text = text;
			this->msLeft = 1500;
		};

		string getText(bool colorful) {
			if (colorful) {
				return "\033[37;40m" + (this->text) + "\033[0m";
			}
			return (this->text);
		};
};

// Storage for notifications, limited to 8 displayed at a time
Notification* notificationsArr[8] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

void addNotification(Notification* notification) {
	// Check if an available slot exists; if so, use it
	for (int i = 0; i < 8; i++) {
		if (!notificationsArr[i]) {
			notificationsArr[i] = notification;
			return;
		}
	}

	// If no available slot exists, free the first slot and shift the rest down by one
	free(notificationsArr[0]);
	for (int i = 0; i < 7; i++) {
		notificationsArr[i] = notificationsArr[i + 1];
	}
	// Then, make the last slot the new notification
	notificationsArr[7] = notification;
}

void updateNotifications(int print, int msElapsed) {
	for (int i = 0; i < 8; i++) {
		if (!notificationsArr[i])	continue; // Make sure this notification slot is not nullptr

		notificationsArr[i]->msLeft -= msElapsed;

		// If a notification has expired
		if (notificationsArr[i]->msLeft <= 0) {
			free(notificationsArr[i]);

			// Shift down the rest of the notifications
			for (int j = i; j < 7; j++) {
				notificationsArr[j] = notificationsArr[j + 1];
			}
			// The last pointer shouldn't be freed since slot 6 now references it
			notificationsArr[7] = nullptr;
		}
	}

	// If printing is enabled
	if (print != 0) {
		cout << "\033[H"; // Set cursor to top-left
		for (int i = 0; i < 8; i++) {
			
			// If this slot is not nullptr
			if (notificationsArr[i]) {
				cout << notificationsArr[i]->getText(print == 2);
			}
			cout << endl << "\r";
		}
	}
}