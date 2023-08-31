// cross_correlation.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "windows.h"
#include <winsock.h>
#pragma comment(lib,"ws2_32.lib") // winsock library 
#include <math.h>

#include "RobotCommSend.h"
#include "RobotCommReceive.h"

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>
#include <string>
#include <chrono>
#include <ctime>

using namespace cv;
using namespace std;// not good, just use std every time.

// Variables related to defining region of interest for template matching
bool first_ROI_defined = false;
bool second_ROI_defined = false;

// Variables related to defining the cropping region for the screen shot
// int clippingRegionX, clippingRegionY, clippingRegionWidth, clippingRegionHeight;
// Pen selectPen;
// bool start = false;

// Vriables for measuring UDP send freq
// clock_t start;
double t_duration;
int counter;

Mat getMat(HWND hWnd, int x1, int y1, int x2, int y2) {
    // HRGN hClpRgn = CreateRectRgn(x1,y1,x2,y2);
    // HDC hScreen = GetDCEx(hWnd, hClpRgn, DCX_INTERSECTRGN);
    HDC hScreen = GetDC(hWnd);
    HDC hDC = CreateCompatibleDC(hScreen);
    
    // RECT windowRect;
    // GetClientRect(hWnd,&windowRect);

    int height = y2 - y1; // windowRect.bottom;
    int width = x2 - x1; // windowRect.right;
    
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    SelectObject(hDC, hBitmap);
    
    // BitBlt(HDC hdc,int x,int y,int cx,int cy,HDC hdcSrc,int x1,int y1,DWORD rop)
    // hdc = A handle to the destination device context
    // x = The x-coordinate, in logical units, of the upper-left corner of the destination rectangle
    // y = The y-coordinate, in logical units, of the upper-left corner of the destination rectangle
    // cx = The width, in logical units, of the source and destination rentangles
    // cy = The height, in logical units, of the source and destination rectangles
    // hdcSrc = A handle to the source device context
    // x1 = The x-coordinate, in logical units, of the upper-left corner of the source rectangle
    // y1 = The y-coordinate, in logical units, of the upper-left corner of the source rectangle
    // rop = A raster-operation code -- SRCCOPY = Copies the source rectangle directly to the destination rectangle
    BitBlt(hDC, 0, 0, width, height, hScreen, x1, y1, SRCCOPY);

    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    Mat mat(height, width, CV_8UC4, Scalar(0, 0, 0, 8));

    GetDIBits(hDC, hBitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(hWnd,hScreen);

    return mat;
}

int main(int argc, char** argv)
{
    // ****** UDP Communications ****** ONLY COMMENTED FOR TESTING ON PERSONAL COMPUTER 

    cu::robotics::RobotCommSend send;
    cu::robotics::RobotCommReceive recv;
    send.initialize("192.168.1.100",27000);
    recv.initialize(27001);

    // ***********************************




    // ******  VARIABLE DECLARATION ****** 

    double udpSendData[2];
    double udpRecvData[2];
    int dx, dz; // Values in px
    double distance_bw_ROIs;

    Point center_of_first_ROI;
    Point center_of_second_ROI;

    // ***********************************




    // ******  GET A SCREENSHOT ****** 

     LPCWSTR windowTitle = L"cross_correlation - Microsoft Visual Studio Current"; // LPCWTSR is a 32-bit pointer to a constant string of 16-bit Unicode characters .. typedef const wchar_t* LPCWSTR
    //LPCWSTR windowTitle = L"20223_08_16_exvivo_pig_cropped.mp4 - VLC media player"; // LPCWTSR is a 32-bit pointer to a constant string of 16-bit Unicode characters .. typedef const wchar_t* LPCWSTR
    HWND hWnd = FindWindow(NULL, windowTitle);

    // ***********************************




    // ****** CLIPPING REGION ******

    // x1 = Specifies the x-coordinate of the upper-left corner of the region in logical units
    // y1 = Specifies the y-coordinate of the upper-left corner of the region in logical units
    // x2 = Specifies the x-coordinate of the lower-right corner of the region in logical units
    // y2 = Specifies the y-coordinate of the lower-right corner of the region in logical units


    // This is the window clipping region
    int x1 = 545; // x-coord upper-left corner // 500
    int y1 = 130; // y-coord upper-left corner // 500
    int x2 = 1425; // x-coord lower-right corner // 1000
    int y2 = 1000; // y-coord lower-right corner // 1000

    //// This is the window clipping region *FOR LOCAL MACHINE TESTING ONLY*
     //int x1 = 0; // x-coord upper-left corner // 500
     //int y1 = 0; // y-coord upper-left corner // 500
     //int x2 = 1500; // x-coord lower-right corner // 1000
     //int y2 = 1500; // y-coord lower-right corner // 1000

    // TEMPORARY DEBUGGING VALUES
     //int x1 = 600; // x-coord upper-left corner // 500
     //int y1 = 100; // y-coord upper-left corner // 500
     //int x2 = 1300; // x-coord lower-right corner // 1000
     //int y2 = 600; // y-coord lower-right corner // 1000

    // ***********************************




    // ****** CREATE NEW WINDOW ******

    String windowName = "Cross-Correlation"; // Name of the window
    namedWindow(windowName, WINDOW_FULLSCREEN); // Create a window

    // ***********************************





    // ******  Second variable declaration for ROI templates and comparisons???? ****** MIGHT COMBIINE WITH THE FIRST SECTION

    vector<Rect> ROIs(2);
    Mat grayScreenshotImage;
    Mat image_for_display;
    Mat first_ROI_template;
    Mat second_ROI_template;
    bool areROIsOverlapped = false;
    Mat result;

    // ***********************************





    // ******  Appears to be some testing on the first ROI comparison ******** NEED TO BETTER UNDERSTAND BEFORE CHANGING

    // double first_ROI_minVal, first_ROI_maxVal;
    // Point first_ROI_minLoc, first_ROI_maxLoc, first_ROI_matchLoc;

    // ***********************************




    // ******* more variables for comparison speciffically for the second ROI values and locations ****** MIGHT COMBIINE WITH THE FIRST SECTION

    double second_ROI_minVal, second_ROI_maxVal;
    Point second_ROI_minLoc, second_ROI_maxLoc, second_ROI_matchLoc;

    // ***********************************




    // ******* MAIN LOOP FOR SEGMENTATION AND ROI COMPARISON ********

    while (waitKey(1) != 113) { // Wait for ESC key press in the window for specified milliseconds -- 0 = forever

        // ****** CLOCK FOR CALCULATING PROGRAM EXECUTION TIME ****** Not necessary for every loop but helpful to have in the program

        // Start the clock
        // start = clock();
        // auto t_start = std::chrono::high_resolution_clock::now();

        // ***********************************




        // ******* UDP MESSAGE RECEIVING *******

        // Receive UDP messages from the target machine 
        recv.receive(udpRecvData); // ******* ONLY COMMENTED FOR TESTING ON PERSONAL COMPUTER *******

        // The recieved UDP packet contains two values
        // (1) dx = change in needle tip position along the x-axis of the image (value in px)
        // (2) dy = change in needle tip position along the y-axis of the image (value in px)

        // ***********************************





        // ******* MESSAGE PROCESSING *******

        // This is some data from the robot, but we havent used it yet (the end effector speed)
        dx = static_cast<int>(udpRecvData[0]);
        dz = static_cast<int>(udpRecvData[1]);

        // TODO: Check the units of these values (assumed px) Yes, conversion done on target machine
        // TODO: Find out a solution to the UDP communication problem

        //cout << dx << "," << dz << endl; // used this while testing the code

        // ***********************************




        // ******* CREATE SCREENSHOT *******

        // Get the screenshot
        Mat screenshotImage = getMat(hWnd, x1, y1, x2, y2); // uses the active window handle and the desired window size (dimensions)

        //// Get a copy of the screenshot for displaying
        //screenshotImage.copyTo(image_for_display);

        image_for_display = screenshotImage.clone();

        // Convert the BGR color screenshot into grayscale screenshot for processing
        cvtColor(screenshotImage, grayScreenshotImage, COLOR_BGR2GRAY);

        // ***********************************




        // ******* IMAGE AUGMENTATIONS (FILTERS) *******

        // Applying Gaussian filter
        // Gaussian filter parameters
        // Default filter (Gaussian kernel) size is defined as: 2*ceil(2*sigma)+1
        // Following section of code mimics MATLAB's imgaussfilt() function

        Mat filteredScreenshotImage;
        Mat cimg;
        double sigma = 1; // Standard deviation of Gaussian filter
        int FilterSize = 2 * (int)ceil(2 * sigma) + 1; // Size of the Gaussian kernel, FilterSize x FilterSize
        GaussianBlur(grayScreenshotImage, filteredScreenshotImage, Size(FilterSize, FilterSize), 0);

        // ***********************************





        // ******* FIRST ROI MATCHING *******

        // Select first ROI

        if (!first_ROI_defined) {
            ROIs[0] = selectROI(windowName, image_for_display, true, false);
            first_ROI_template = filteredScreenshotImage(ROIs[0]);
            first_ROI_defined = true;
        }
        // ***********************************






        // ******* Needle Tip Overlay *******

        //Mat thresh_ROI = first_ROI_template > 100;

        //for (uint16_t j = 0; j < ROIs[0].height; ++j) {
        //    for (uint16_t i = 0; i < ROIs[0].width; ++i) {
        //       

        //        if (thresh_ROI.at<bool>(j, i) == 255) {

        //            
        //            //image_for_display.at<Vec3b>(ROIs[0].y+ j, 1.35*ROIs[0].x + i) = Vec3b(255, 255, 255);
        //            image_for_display.at<Vec3b>(Point(ROIs[0].x + i, ROIs[0].y + j)) = Vec3b(255, 255, 255);

        //            //image_for_display[ROIs[0].y + j][ROIs[0].x + i] = (255, 255, 255);


        //            /*cout << "point(" << ROIs[0].x + i << "," << ROIs[0].y + j << endl;
        //            if (i % 10 == 0) {
        //                circle(image_for_display, Point(ROIs[0].x+i, ROIs[0].y+j), 10, Scalar(0, 255, 0), 2);
        //            }*/
        //        }         
        //    }

        //}


        //imshow(windowName, image_for_display);
        
        // ***********************************





        // Template matching - First ROI ******* MIGHT NOT NEED THIS IF THE RIGID TIP KEEPS THE NEEDLE FROM MOVING *******

        // matchTemplate(filteredScreenshotImage, first_ROI_template, result, TM_CCORR_NORMED);
        // normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());
        // minMaxLoc(result, &first_ROI_minVal, &first_ROI_maxVal, &first_ROI_minLoc, &first_ROI_maxLoc, Mat());
        // first_ROI_matchLoc = first_ROI_maxLoc;
        // rectangle(image_for_display, first_ROI_matchLoc, Point(first_ROI_matchLoc.x + first_ROI_template.cols, first_ROI_matchLoc.y + first_ROI_template.rows), Scalar(0, 0, 255), 3, 8, 0);

        //rectangle(image_for_display, Point(ROIs[0].x, ROIs[0].y), Point(ROIs[0].x + ROIs[0].width, ROIs[0].y + ROIs[0].height), Scalar(0, 0, 255), 3, 8, 0);
        rectangle(image_for_display, Point(ROIs[0].x, ROIs[0].y), Point(ROIs[0].x + ROIs[0].width, ROIs[0].y + ROIs[0].height), Scalar(0, 0, 255), 3, 8, 0);


        // ***********************************





        // ******* SECOND ROI MATCHING *******

        // Select second ROI

        if (!second_ROI_defined) {
            ROIs[1] = selectROI(windowName, image_for_display, true, false);
            second_ROI_template = filteredScreenshotImage(ROIs[1]);
            second_ROI_defined = true;
        }

        // Template matching - Second ROI

        if (!areROIsOverlapped) {
            matchTemplate(filteredScreenshotImage, second_ROI_template, result, TM_CCORR_NORMED);
            normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());
            minMaxLoc(result, &second_ROI_minVal, &second_ROI_maxVal, &second_ROI_minLoc, &second_ROI_maxLoc, Mat());
            second_ROI_matchLoc = second_ROI_maxLoc;
        }
        rectangle(image_for_display, second_ROI_matchLoc, Point(second_ROI_matchLoc.x + second_ROI_template.cols, second_ROI_matchLoc.y + second_ROI_template.rows), Scalar(255, 0, 0), 3, 8, 0);

        // ***********************************






        // Define the center of the first region of interest 

        // center_of_first_ROI.x = first_ROI_matchLoc.x + first_ROI_template.cols / 2; // USE THIS IF USING TEMPLATE MATCHING WITH A MOBILE FIRST ROI
        // center_of_first_ROI.y = first_ROI_matchLoc.y + first_ROI_template.rows / 2;

        center_of_first_ROI.x = ROIs[0].x + ROIs[0].width / 2; // USE THIS IF ASSUMING THE NEEDLE TIP IS FIXED
        center_of_first_ROI.y = ROIs[0].y + ROIs[0].height / 2;

        // Define the center of the second region of interest
        center_of_second_ROI.x = second_ROI_matchLoc.x + second_ROI_template.cols / 2;
        center_of_second_ROI.y = second_ROI_matchLoc.y + second_ROI_template.rows / 2;

        // Compute the distance between the centers of these regions of interest
        distance_bw_ROIs = cv::norm(center_of_first_ROI - center_of_second_ROI);

        // Determine whether needle has entered the first region of interest or not and if it has then handle it accordingly
        if (distance_bw_ROIs < second_ROI_template.cols / 2 || distance_bw_ROIs < second_ROI_template.rows / 2) {
            areROIsOverlapped = true;
            second_ROI_matchLoc.x += dx;
            second_ROI_matchLoc.y += dz;
        }
        else {
            areROIsOverlapped = false;
        }


        // Display the image
        imshow(windowName, image_for_display); // Show the image inside the created window
        //imshow(windowName, cimg); // Show the image inside the created window



        // Send the distance between these two ROIs over UDP

        udpSendData[0] = std::sqrt((center_of_second_ROI.x - center_of_first_ROI.x) * (center_of_second_ROI.x - center_of_first_ROI.x));
        udpSendData[1] = std::sqrt((center_of_second_ROI.y - center_of_first_ROI.y) * (center_of_second_ROI.y - center_of_first_ROI.y));
        send.send(udpSendData); // ******* ONLY COMMENTED FOR TESTING ON PERSONAL COMPUTER *******
        




        // ******* CALCULATE EXECUTION TIME *******

        // Calculate the duaration for the executing one cycle of while loop
        // t_duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t_start).count();

        // Update the counter
        // counter++;

        // print the freq of loop execution
        /*if (counter == 20) {
            cout << 1000 / t_duration << endl;
            counter = 0;
        }*/

        // ***********************************


    }

    // ***********************************







    destroyWindow(windowName); // Destroy the created window

    return 0;
}
 
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file