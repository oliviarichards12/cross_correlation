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


bool local_testing = true;


// Variables related to defining region of interest for template matching
bool first_ROI_defined = false;
bool second_ROI_defined = false;


// Vriables for measuring UDP send freq
// clock_t start;
double t_duration;
int counter=0;

Mat getMat(HWND hWnd, int x1, int y1, int x2, int y2) {

    HDC hScreen = GetDC(hWnd);
    HDC hDC = CreateCompatibleDC(hScreen);
    

    int height = y2 - y1; // windowRect.bottom;
    int width = x2 - x1; // windowRect.right;
    
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    SelectObject(hDC, hBitmap);
    
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

    // ******  VARIABLE DECLARATION ****** 

    // UDP com
    cu::robotics::RobotCommSend send;
    cu::robotics::RobotCommReceive recv;
    double udpSendData[2];
    double udpRecvData[2];
    int dx, dz; // Values in px
    
    // ROI definition
    Point center_of_first_ROI;
    Point center_of_second_ROI;
    vector<Rect> ROIs(2);
    Mat first_ROI_template;
    Mat second_ROI_template;
    bool areROIsOverlapped = false;
    Mat result;
    double distance_bw_ROIs;

    // ROI correlation variables (currently not using first ROI corr. var.)
    // double first_ROI_minVal, first_ROI_maxVal;
    // Point first_ROI_minLoc, first_ROI_maxLoc, first_ROI_matchLoc;
    double second_ROI_minVal, second_ROI_maxVal;
    Point second_ROI_minLoc, second_ROI_maxLoc, second_ROI_matchLoc;
    int margin_size;
    Rect enlarged_second_ROI;
    Mat second_ROI_search_region;

    // Clipping region
    int x1;
    int x2;
    int y1;
    int y2;

    // Image capture
    Mat screenshotImage;
    Mat grayScreenshotImage;
    Mat image_for_display;


    // ***********************************





    // ****** UDP Communications ****** 
    /*
        Note:
            - Figure out what the syntax is for using UDP connection. What is the IP adress for?? How to set it??
    */

    if (!local_testing) {
        send.initialize("192.168.1.100", 25041);
        recv.initialize(27001);
    }


    // ***********************************



    // ******  GET A SCREENSHOT ****** 
    /*
        Notes:
            - LPCWTSR is a 32-bit pointer to a constant string of 16-bit Unicode characters .. typedef const wchar_t* LPCWSTR
    */
    LPCWSTR windowTitle;

    if (local_testing) {
        windowTitle = L"20223_08_16_exvivo_pig_cropped.mp4 - VLC media player";
    }
    else {
        windowTitle = L"ThorImageOCT 5.2.1 - Vega";
    }
    
    HWND hWnd = FindWindow(NULL, windowTitle);

    // ***********************************




    // ****** CLIPPING REGION ******
    /*
        Notes: 
            - Description: 
                x1 = Specifies the x-coordinate of the upper-left corner of the region in logical units
                y1 = Specifies the y-coordinate of the upper-left corner of the region in logical units
                x2 = Specifies the x-coordinate of the lower-right corner of the region in logical units
                y2 = Specifies the y-coordinate of the lower-right corner of the region in logical units
    */



    if (local_testing) {
        x1 = 0; // x-coord upper-left corner // 500
        y1 = 0; // y-coord upper-left corner // 500
        x2 = 1500; // x-coord lower-right corner // 1000
        y2 = 1500; // y-coord lower-right corner // 1000
    }
    else {
        x1 = 450; // x-coord upper-left corner // 500
        y1 = 200; // y-coord upper-left corner // 500
        x2 = 1200; // x-coord lower-right corner // 1000
        y2 = 1000; // y-coord lower-right corner // 1000
    }

    // ***********************************




    // ****** CREATE NEW WINDOW ******

    String windowName = "Cross-Correlation"; // Name of the window
    namedWindow(windowName, WINDOW_FULLSCREEN); // Create a window

    // ***********************************

 


    // ******* MAIN LOOP FOR SEGMENTATION AND ROI COMPARISON ********

    while (waitKey(1) != 113) { // Wait for ESC key press in the window for specified milliseconds -- 0 = forever
        

        // ****** CLOCK FOR CALCULATING PROGRAM EXECUTION TIME ****** 
        /*
            Notes:
                - Not necessary for every loop but helpful to have in the program
        */

        // Start the clock
        // start = clock();
        // auto t_start = std::chrono::high_resolution_clock::now();

        // ***********************************




        // ******* UDP MESSAGE RECEIVING *******
        /*
            Notes:
                - COMMENT FOR TESTING ON PERSONAL COMPUTER

                - The recieved UDP packet contains two values
                    (1) dx = change in needle tip position along the x-axis of the image (value in px)
                    (2) dy = change in needle tip position along the y-axis of the image (value in px)

        */

        if (!local_testing) {
            // Receive UDP messages from the target machine
            recv.receive(udpRecvData);
        }
         

        // ***********************************





        // ******* MESSAGE PROCESSING *******
        /*
            Notes:
                - IMPLEMENT ERROR HANDLING:
                    If the packet size isn't the correct size, assign dx/dz to 0
        */

        if (local_testing) {
            dx = 0;
            dz = 0;
        }
        else {
            // This is some data from the robot, but we havent used it yet (the end effector speed)
            dx = static_cast<int>(udpRecvData[0]);
            dz = static_cast<int>(udpRecvData[1]);

            //if (counter == 100) {
            //    cout << dx << "," << dz << endl; // used this while testing the code
            //}
        }

        // ***********************************




        // ******* CREATE SCREENSHOT *******
         
        // Get the screenshot
        Mat screenshotImage = getMat(hWnd, x1, y1, x2, y2); // uses the active window handle and the desired window size (dimensions)

        // Get a copy of the screenshot for displaying
        screenshotImage.copyTo(image_for_display);

        // Convert the BGR color screenshot into grayscale screenshot for processing
        cvtColor(screenshotImage, grayScreenshotImage, COLOR_BGR2GRAY);

        // ***********************************




        // ******* IMAGE AUGMENTATIONS (FILTERS) *******
        /*
            Notes: 
               - Uses bilateral filter instead of GaussianBlur. Thresholds image to increase layer definition.
        
        */
        Mat filteredScreenshotImage;
        Mat image_medblur;
        Mat image_bifilter;

        bilateralFilter(grayScreenshotImage, image_bifilter, 15, 80, 80);
        threshold(image_bifilter, filteredScreenshotImage, 95, 0, THRESH_TOZERO);
       
        // Troubleshooting: shows the image that is being used to do the cross-correlation
        //imshow("Test_Image", filteredScreenshotImage);


        // ***********************************





        // ******* FIRST ROI SETUP *******
        /*
            Notes: 
                - TRY TEMPLATE UPDATING
        */

        if (!first_ROI_defined) {
            ROIs[0] = selectROI(windowName, image_for_display, true, false);
            first_ROI_template = filteredScreenshotImage(ROIs[0]);
            first_ROI_defined = true;
        }

        // ***********************************






        // ******* NEEDLE TIP OVERLAY *******
        /*
            Notes: 
                - NEEDS WORK. X-OFFSET STILL INCORRECT
        
        */

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





        // ******* TEMPLATE MATCHING: FIRST ROI ******* 
        /*
            Notes: 
                - MIGHT NOT NEED THIS IF THE RIGID TIP KEEPS THE NEEDLE FROM MOVING
        */

        // matchTemplate(filteredScreenshotImage, first_ROI_template, result, TM_CCORR_NORMED);
        // normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());
        // minMaxLoc(result, &first_ROI_minVal, &first_ROI_maxVal, &first_ROI_minLoc, &first_ROI_maxLoc, Mat());
        // first_ROI_matchLoc = first_ROI_maxLoc;
        // rectangle(image_for_display, first_ROI_matchLoc, Point(first_ROI_matchLoc.x + first_ROI_template.cols, first_ROI_matchLoc.y + first_ROI_template.rows), Scalar(0, 0, 255), 3, 8, 0);

        rectangle(image_for_display, Point(ROIs[0].x, ROIs[0].y), Point(ROIs[0].x + ROIs[0].width, ROIs[0].y + ROIs[0].height), Scalar(0, 0, 255), 3, 8, 0);


        // ***********************************





        // ******* SECOND ROI SETUP *******

        if (!second_ROI_defined) {
            ROIs[1] = selectROI(windowName, image_for_display, true, false);
            second_ROI_template = filteredScreenshotImage(ROIs[1]);
            second_ROI_matchLoc.x = ROIs[1].x;
            second_ROI_matchLoc.y = ROIs[1].y;
            second_ROI_defined = true;
        }

        // ***********************************


        
       
        // ******* TEMPLATE MATCHING: SECOND ROI ******* 
        /*
            Notes: 
                - The search region needs to be offset by the robot movement sent through the UDP.
                
        */

        margin_size = 50; // number of pixels around the ROI to search for a match adjusted according to the robot motion
        enlarged_second_ROI = Rect(Point(second_ROI_matchLoc.x, second_ROI_matchLoc.y - margin_size + dz), Point(second_ROI_matchLoc.x + ROIs[1].width, second_ROI_matchLoc.y + ROIs[1].height + margin_size + dz));
        second_ROI_search_region = filteredScreenshotImage(enlarged_second_ROI);
        
        if (!areROIsOverlapped) {
            matchTemplate(second_ROI_search_region, second_ROI_template, result, TM_CCORR_NORMED);
            //normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());
            minMaxLoc(result, &second_ROI_minVal, &second_ROI_maxVal, &second_ROI_minLoc, &second_ROI_maxLoc, Mat());
            if (counter %10 == 0) {
                cout << "Match score: "<< second_ROI_maxVal << endl; 
            }
            second_ROI_matchLoc = second_ROI_maxLoc + Point(second_ROI_matchLoc.x, second_ROI_matchLoc.y - margin_size);
            
            // template updating
            if(counter % 10 == 0 && second_ROI_maxVal > 0.8){
                second_ROI_template = filteredScreenshotImage(Rect(Point(second_ROI_matchLoc.x, second_ROI_matchLoc.y), Point(second_ROI_matchLoc.x + ROIs[1].width, second_ROI_matchLoc.y + ROIs[1].height)));
            }
            
        }
        rectangle(image_for_display, second_ROI_matchLoc, Point(second_ROI_matchLoc.x + second_ROI_template.cols, second_ROI_matchLoc.y + second_ROI_template.rows), Scalar(255, 0, 0), 3, 8, 0);

        
        // ***********************************




        // ******* CHECK IF ROIS OVERLAP *******

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
            //areROIsOverlapped = true;
            areROIsOverlapped = false;

            second_ROI_matchLoc.x += dx;
            second_ROI_matchLoc.y += dz;
        }
        else {
            areROIsOverlapped = false;
        }

        // ***********************************




        // ******* DISPLAY IMAGE *******

        imshow(windowName, image_for_display); // Show the image inside the created window
        //imshow(windowName, cimg); // Show the image inside the created window

        // ***********************************




        // ******* UDP SEND *******
        
        //if (!local_testing) {
        //    // Sends the distance between these two ROIs over UDP
        //    udpSendData[0] = std::sqrt((center_of_second_ROI.x - center_of_first_ROI.x) * (center_of_second_ROI.x - center_of_first_ROI.x));
        //    udpSendData[1] = std::sqrt((center_of_second_ROI.y - center_of_first_ROI.y) * (center_of_second_ROI.y - center_of_first_ROI.y));
        //    send.send(udpSendData);
        //}
                
        // ***********************************



        // ******* CALCULATE EXECUTION TIME *******

        // Calculate the duaration for the executing one cycle of while loop
        // t_duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t_start).count();

        // Update the counter
        counter++;

        // print the freq of loop execution
        if (counter == 101) {
            //cout << 1000 / t_duration << endl;
            counter = 0;
        }

        // ***********************************


    }

    // ***********************************



    destroyWindow(windowName); // Destroy the created window

    return 0;
}
 