# Document Scanner

This is a simple desktop application written in C++ that allows you to scan documents from images.

The program uses OpenCV to detect a document in a photo, correct its perspective, and save it as a clean, flat image. A basic graphical interface is implemented using wxWidgets.

## Features

* Load images (`.jpg`, `.png`)
* Automatic document detection
* Perspective correction (warp)
* Three processing modes:

  * Original
  * Grayscale
  * Black & White
* Saves result to a `results` folder

## How to use

1. Run the application
2. Click "Choose file"
3. Select an image with a document
4. Choose processing mode
5. The result will be saved automatically

## Requirements

* C++ (C++17 or higher)
* OpenCV
* wxWidgets

## Notes

The app works best with clear images where the document edges are visible.
