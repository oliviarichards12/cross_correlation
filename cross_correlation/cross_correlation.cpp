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
#include <opencv2/imgproc/segmentation.hpp>

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


// Variables for measuring UDP send freq
// clock_t start;
double t_duration;
int counter=0;

// Intelligent Scissor variables 
int x_mouse_move; 
int y_mouse_move; 
int click_num; 
int startX;
int startY;
int endX; 
int endY;
std::vector<cv::Mat> border_contours;
int box_size_x = 40;
int box_size_y = box_size_x * 1.5;





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

void CallBackFunc(int event, int x, int y, int flags, void* userdata) // https://www.opencv-srf.com/2011/11/mouse-events.html
{
    if(!second_ROI_defined){
        if (event == EVENT_LBUTTONDOWN)
        {
            click_num += 1;
            if (click_num == 1) {
                startX = x;
                startY = y;
                cout << "1st Left button of the mouse is clicked - position(" << x << ", " << y << ")" << endl;
            }
            else if (click_num == 2) {
                endX = x; 
                endY = y; 
                click_num = 0;
                cout << "2nd Left button of the mouse is clicked - position(" << x << ", " << y << ")" << endl;
            }
                

            /*cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;*/

        }
        else if (event == EVENT_RBUTTONDOWN) // 
        {
            
        }
        else if (event == EVENT_MOUSEMOVE)
        {
            if (click_num == 1) {
                x_mouse_move = x; 
                y_mouse_move = y;
                cout << "Current position of the mouse (" << x << ", " << y << ")" << endl;
            }

        }
    }
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
    vector<Rect> ROIs(3);
    Mat first_ROI_template;
    Mat second_ROI_template;
    vector<Mat> second_ROI_startEnd_template(2);


    


    Mat temp_ROI_template;
    bool areROIsOverlapped = false;
    Mat result;
    double distance_bw_ROIs;
    double template_error;
    double threshold_error;
    double first_error; 

    // ROI correlation variables (currently not using first ROI corr. var.)
    // double first_ROI_minVal, first_ROI_maxVal;
    Point first_ROI_minLoc, first_ROI_maxLoc, first_ROI_matchLoc;
    double second_ROI_minVal, second_ROI_maxVal;
    Point second_ROI_minLoc, second_ROI_maxLoc, second_ROI_matchLoc;
    vector<Point> second_ROI_startEnd_maxLoc(2); 
    vector<Point> second_ROI_startEnd_matchLoc(2);

    int margin_size;
    Rect enlarged_second_ROI;
    Mat second_ROI_search_region;



    // Image capture
    Mat screenshotImage;
    Mat grayScreenshotImage;
    Mat image_for_display;


    // Clipping region
    int x1;
    int x2;
    int y1;
    int y2;

    // ***********************************





    // ****** UDP Communications ****** 
    /*
        Note:
            - Figure out what the syntax is for using UDP connection. What is the IP adress for?? How to set it??
    */

    if (!local_testing) {
        send.initialize("192.168.1.100", 25041);
        //recv.initialize(27001);
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
            //recv.receive(udpRecvData);
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
            first_ROI_matchLoc.x = ROIs[0].x;
            first_ROI_matchLoc.y = ROIs[0].y;
            first_ROI_template = filteredScreenshotImage(ROIs[0]);
            first_ROI_defined = true;
        }

        // ***********************************






        // ******* NEEDLE TIP OVERLAY *******
        /*
            Notes: 
                - NEEDS WORK. X-OFFSET STILL INCORRECT
        
        */
        
        temp_ROI_template = filteredScreenshotImage(ROIs[0]); // Get the current image's ROI around the needle
        Mat thresh_ROI_high = first_ROI_template > 180; // Threshold the saved image's ROI at a high and low threshold
        Mat thresh_temp_high = temp_ROI_template > 180;// Threshold the current image's ROI at a high and low threshold
        Mat thresh_ROI_low = first_ROI_template > 80;
        Mat thresh_temp_low = temp_ROI_template > 80;
        Mat thresh_ROI;

        uint16_t high_temp_thresh_total = 0;
        uint16_t high_first_thresh_total = 0;
        uint16_t low_temp_thresh_total = 0;
        uint16_t low_first_thresh_total = 0;

        for (uint16_t j = 0; j < ROIs[0].height; ++j) {
            for (uint16_t i = 0; i < ROIs[0].width; ++i) {
                if (thresh_temp_high.at<bool>(j, i) == 255) {
                    high_temp_thresh_total += 1; // Count the pixels with a high threshold value
                }
                if (thresh_ROI_high.at<bool>(j, i) == 255) {
                    high_first_thresh_total += 1;
                }

                if (thresh_temp_low.at<bool>(j, i) == 255) {
                    low_temp_thresh_total += 1; // Count the pixels with a low threshold value
                }
                if (thresh_ROI_low.at<bool>(j, i) == 255) {
                    low_first_thresh_total += 1;
                }

                //cout << "first_thresh: " << first_thresh_total << endl;
            }
        }
        
        template_error = abs(high_first_thresh_total - high_temp_thresh_total) / high_first_thresh_total; // if the needle is in the frame, but in a different spot, the error should be low
        threshold_error = abs(high_temp_thresh_total - low_temp_thresh_total) / (high_temp_thresh_total+1); // if changing the threshold doesn't drastically change the number of included pixels (would occur in the tissue)
        first_error = abs(high_first_thresh_total - low_first_thresh_total) / (high_first_thresh_total);

        // if the total white pixels are similar AND the new template doesn't have more white pixels than the original AND if the difference between thresholding values isn't too big
        if (template_error < 0.01 && high_first_thresh_total<high_temp_thresh_total && threshold_error <= first_error) { 
            thresh_ROI = thresh_temp_low;
        }
        else {
            // TODO: move the original overlay ROI according to the UDP packets
            thresh_ROI = thresh_ROI_low;
        }
        

        for (uint16_t j = 0; j < ROIs[0].height; ++j) {
            for (uint16_t i = 0; i < ROIs[0].width; ++i) {
               

                if (thresh_ROI.at<bool>(j, i) == 255) {

                    
                    image_for_display.at<Vec3b>(ROIs[0].y+ j, 1.35*ROIs[0].x + i) = Vec3b(255, 255, 255);
                    //image_for_display.at<Vec3b>(Point(ROIs[0].x + i, ROIs[0].y + j)) = Vec3b(255, 255, 255);

                    //image_for_display[ROIs[0].y + j][ROIs[0].x + i] = (255, 255, 255);


                    /*cout << "point(" << ROIs[0].x + i << "," << ROIs[0].y + j << endl;
                    if (i % 10 == 0) {
                        circle(image_for_display, Point(ROIs[0].x+i, ROIs[0].y+j), 10, Scalar(0, 255, 0), 2);
                    }*/
                }         
            }

        }


        imshow(windowName, image_for_display);
        
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





        //// ******* SECOND ROI SETUP *******

        //if (!second_ROI_defined) {
        //    ROIs[1] = selectROI(windowName, image_for_display, true, false);
        //    second_ROI_template = filteredScreenshotImage(ROIs[1]);
        //    second_ROI_matchLoc.x = ROIs[1].x;
        //    second_ROI_matchLoc.y = ROIs[1].y;
        //    second_ROI_defined = true;
        //}
        //// ***********************************



        //// ******* TEMPLATE MATCHING: SECOND ROI ******* 
        ///*
        //    Notes:
        //        - The search region needs to be offset by the robot movement sent through the UDP.

        //*/

        //margin_size = 50; // number of pixels around the ROI to search for a match adjusted according to the robot motion
        //enlarged_second_ROI = Rect(Point(second_ROI_matchLoc.x, second_ROI_matchLoc.y - margin_size + dz), Point(second_ROI_matchLoc.x + ROIs[1].width, second_ROI_matchLoc.y + ROIs[1].height + margin_size + dz));
        //second_ROI_search_region = filteredScreenshotImage(enlarged_second_ROI);

        //if (!areROIsOverlapped) {
        //    matchTemplate(second_ROI_search_region, second_ROI_template, result, TM_CCORR_NORMED);
        //    //normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());
        //    minMaxLoc(result, &second_ROI_minVal, &second_ROI_maxVal, &second_ROI_minLoc, &second_ROI_maxLoc, Mat());
        //    if (counter % 10 == 0) {
        //        cout << "Match score: " << second_ROI_maxVal << endl;
        //    }
        //    second_ROI_matchLoc = second_ROI_maxLoc + Point(second_ROI_matchLoc.x, second_ROI_matchLoc.y - margin_size);

        //    // template updating
        //    if (counter % 10 == 0 && second_ROI_maxVal > 0.8) {
        //        second_ROI_template = filteredScreenshotImage(Rect(Point(second_ROI_matchLoc.x, second_ROI_matchLoc.y), Point(second_ROI_matchLoc.x + ROIs[1].width, second_ROI_matchLoc.y + ROIs[1].height)));
        //    }

        //}
        //rectangle(image_for_display, second_ROI_matchLoc, Point(second_ROI_matchLoc.x + second_ROI_template.cols, second_ROI_matchLoc.y + second_ROI_template.rows), Scalar(255, 0, 0), 3, 8, 0);


        //// ***********************************













        // ******* SECOND ROIs CENTERLINE SETUP WITH INTELLIGENT SCISSORS *******

        if (!second_ROI_defined) {
            
            click_num = 0;
            segmentation::IntelligentScissorsMB tool;
            tool.setEdgeFeatureCannyParameters(32, 100) // possilby use the ROIs[1].x,y for the threshold parameters
            .setGradientMagnitudeMaxLimit(200);
            // calculate image features
            tool.applyImage(filteredScreenshotImage);


            setMouseCallback(windowName, CallBackFunc, NULL);
            //imshow(windowName, image_for_display);
            
            bool hasMap = false;
            Mat dst;
            

            while (waitKey(1) != 13) { // press enter to escape
                


                // calculate map for specified source point
                if (!hasMap && click_num==1) {
                    Point source_point(startX, startY);
                    tool.buildMap(source_point);
                    hasMap = true;
                }
                

                // fast fetching of contours
                // for specified target point and the pre-calculated map (stored internally)
                if (hasMap && click_num == 1) {
                    image_for_display.copyTo(dst);

                    Point target_point(x_mouse_move, y_mouse_move);
                    cv::Mat contour;

                    tool.getContour(target_point, contour);
                    std::vector<cv::Mat> contours;
                    contours.push_back(contour);

                    cv::Scalar color(0, 255, 0, 255);
                    cv::polylines(dst, contours, false, color, 5, cv::LINE_8);

                    //imshow("Intelligent Scissors", dst);
                    imshow(windowName, dst);
                    border_contours = contours;

                }

                if (click_num == 0) {
                    hasMap = false;
                }
                 
            }
            
            //for (size_t i = 0; i < border_contours.size(); ++i) {
            //    // Print information about the cv::Mat (optional)
            //    std::cout << "Mat " << i << ":\n";
            //    std::cout << "Size: " << border_contours[i].size() << std::endl;
            //    std::cout << "Channels: " << border_contours[i].channels() << std::endl;
            //    std::cout << "Type: " << border_contours[i].type() << std::endl;

            //    // Visualize the cv::Mat (optional)
            //    std::cout << border_contours[i] << endl;
            //     // Wait for a key press before closing the window
            //}

            //second_ROI_defined = true;
            
            
        

        // ***********************************




        // ******* SEGMENT DISCRETIZING: SECOND ROIs USING INTELLIGENT SCISSORS *******

        std::vector<cv::Point> contour = border_contours[0];

        // Calculate the arc length
        double contourLength = cv::arcLength(contour, false);

        // Calculate the length of each discretized segment (10 ROIs total)
        double segmentLength = contourLength / 11.0;

        // Create the segments
        std::vector<cv::Point> segmentPoints;
        segmentPoints.push_back(contour[0]); 
        double accumulatedLength = 0.0;

        for (size_t i = 1; i < contour.size(); i++) {
            double segment = cv::norm(contour[i] - contour[i - 1]);
            accumulatedLength += segment;

            if (accumulatedLength >= segmentLength) {
                segmentPoints.push_back(contour[i]);
                accumulatedLength = 0.0;
            }

            if (segmentPoints.size() == 10) break;
        }

        // Ensure we have exactly 10 points
        if (segmentPoints.size() < 11) {
            segmentPoints.push_back(contour.back());
        }

        // Create ROIs
        int margin = 50;
        std::vector<cv::Rect> rois;

        for (size_t i = 0; i < segmentPoints.size() - 1; i++) {
            cv::Point p1 = segmentPoints[i];
            cv::Point p2 = segmentPoints[i + 1];

            int minX = std::min(p1.x, p2.x);
            int minY = std::min(p1.y, p2.y) - margin;
            int maxX = std::max(p1.x, p2.x);
            int maxY = std::max(p1.y, p2.y) + margin;

            cv::Rect roi(minX, minY, maxX - minX, maxY - minY);
            rois.push_back(roi);
        }

        //// Draw the ROIs on an output image for visualization
        //Mat outputImage; 
        //image_for_display.copyTo(outputImage);

        //for (const auto& roi : rois) {
        //    cv::rectangle(outputImage, roi, cv::Scalar(0, 255, 0), 2);
        //}


        //cv::polylines(outputImage, border_contours, false, cv::Scalar(0, 255, 0), 5, cv::LINE_8);

        //// Display the result
        //cv::imshow("ROIs", filteredScreenshotImage);
        //cv::waitKey(0);

        // ***********************************




            
        Point p1(startX - box_size_x, startY - box_size_y);
        Point p2(startX + box_size_x, startY + box_size_y);
        Rect temp_roi(p1, p2);
        ROIs[1] = temp_roi;

        p1 = Point(endX - box_size_x, endY - box_size_y);
        p2 = Point(endX + box_size_x, endY + box_size_y);
        temp_roi = Rect(p1, p2);
        ROIs[2] = temp_roi;

        second_ROI_startEnd_template[0] = filteredScreenshotImage(ROIs[1]);
        second_ROI_startEnd_template[1] = filteredScreenshotImage(ROIs[2]);

        second_ROI_startEnd_matchLoc[0].x = ROIs[1].x;
        second_ROI_startEnd_matchLoc[0].y = ROIs[1].y;

        second_ROI_startEnd_matchLoc[1].x = ROIs[2].x;
        second_ROI_startEnd_matchLoc[1].y = ROIs[2].y;

        second_ROI_defined = true;

        //// Draw the ROIs on an output image for visualization
        //Mat outputImage; 
        //image_for_display.copyTo(outputImage);

        //for (const auto& roi : ROIs) {
        //    cv::rectangle(outputImage, roi, cv::Scalar(0, 255, 0), 2);
        //}


        //cv::polylines(outputImage, border_contours, false, cv::Scalar(0, 255, 0), 5, cv::LINE_8);

        //// Display the result
        //cv::imshow("ROIs", outputImage);
        //cv::waitKey(0);
        }





        // ******* TEMPLATE MATCHING: SECOND ROIs USING INTELLIGENT SCISSORS ******* 
        /*
            Notes:
                - The search region needs to be offset by the robot movement sent through the UDP.

        */
        int margin_size = 50;

        for (int i = 1; i < 3; i++) {
            enlarged_second_ROI = Rect(Point(second_ROI_startEnd_matchLoc[i-1].x, second_ROI_startEnd_matchLoc[i-1].y - margin_size + dz), Point(second_ROI_startEnd_matchLoc[i-1].x + ROIs[i].width, second_ROI_startEnd_matchLoc[i-1].y + ROIs[i].height + margin_size + dz));
            second_ROI_search_region = filteredScreenshotImage(enlarged_second_ROI);
            
            if (!areROIsOverlapped) {
                matchTemplate(second_ROI_search_region, second_ROI_startEnd_template[i-1], result, TM_CCORR_NORMED);
                //normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());
                minMaxLoc(result, &second_ROI_minVal, &second_ROI_maxVal, &second_ROI_minLoc, &second_ROI_maxLoc, Mat());
                if (counter % 10 == 0) {
                    cout << "Match score: " << second_ROI_maxVal << endl;
                }
                second_ROI_startEnd_matchLoc[i-1] = second_ROI_maxLoc + Point(second_ROI_startEnd_matchLoc[i - 1].x, second_ROI_startEnd_matchLoc[i - 1].y - margin_size);

                // template updating
                if (counter % 10 == 0 && second_ROI_maxVal > 0.9) {
                    second_ROI_startEnd_template[i-1] = filteredScreenshotImage(Rect(Point(second_ROI_startEnd_matchLoc[i - 1].x, second_ROI_startEnd_matchLoc[i - 1].y), Point(second_ROI_startEnd_matchLoc[i - 1].x + ROIs[i].width, second_ROI_startEnd_matchLoc[i - 1].y + ROIs[i].height)));
                }

            }
            rectangle(image_for_display, second_ROI_startEnd_matchLoc[i - 1], Point(second_ROI_startEnd_matchLoc[i - 1].x + second_ROI_startEnd_template[i-1].cols, second_ROI_startEnd_matchLoc[i - 1].y + second_ROI_startEnd_template[i - 1].rows), Scalar(255, 0, 0), 3, 8, 0);

        }



        segmentation::IntelligentScissorsMB tool;
        tool.setEdgeFeatureCannyParameters(32, 100)
            .setGradientMagnitudeMaxLimit(200);
        // calculate image features

        enlarged_second_ROI = Rect(Point(second_ROI_startEnd_matchLoc[0].x, second_ROI_startEnd_matchLoc[0].y - margin_size + dz), Point(second_ROI_startEnd_matchLoc[1].x + ROIs[2].width, second_ROI_startEnd_matchLoc[1].y + ROIs[2].height + margin_size + dz));
        second_ROI_search_region = filteredScreenshotImage(enlarged_second_ROI);

        tool.applyImage(second_ROI_search_region);
 
        //tool.buildMap(Point(second_ROI_startEnd_matchLoc[1].x + box_size_x, second_ROI_startEnd_matchLoc[1].y + box_size_y));

        Point startPoint(box_size_x, margin_size + box_size_y);
        tool.buildMap(startPoint);

        vector<Point> contour_roi;

        Point endPoint(second_ROI_startEnd_matchLoc[1].x - second_ROI_startEnd_matchLoc[0].x + box_size_x, margin_size + box_size_y);
        tool.getContour(endPoint, contour_roi);
        //std::vector<cv::Mat> contours_;
        //contours_.push_back(contour_);

        vector<Point> offsetContour; 
        int offsetX = second_ROI_startEnd_matchLoc[0].x; 
        int offsetY = second_ROI_startEnd_matchLoc[0].y-margin_size;

        for (const auto& point : contour_roi) {
            Point offset = point + Point(offsetX, offsetY);
            offsetContour.push_back(offset);
        }

        cv::Scalar color(0, 255, 0, 255);
        cv::polylines(image_for_display, offsetContour, false, color, 5, cv::LINE_8);

        //imshow("Intelligent Scissors", dst);
        imshow(windowName, image_for_display);







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

            //second_ROI_matchLoc.x += dx; // Commented for testing UDP-influenced template moving for first_ROI 
            //second_ROI_matchLoc.y += dz;
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
 