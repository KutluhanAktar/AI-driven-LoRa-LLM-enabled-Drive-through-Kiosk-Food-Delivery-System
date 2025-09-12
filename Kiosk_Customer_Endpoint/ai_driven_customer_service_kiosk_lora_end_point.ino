         /////////////////////////////////////////////  
        //   AI-driven LoRa & LLM-enabled Kiosk    //
       //         & Food Delivery System          //
      //             ---------------             //
     //  (RA-08H LoRaWan Node Board w/ RP2040)  //           
    //             by Kutluhan Aktar           // 
   //                                         //
  /////////////////////////////////////////////

//
// A research project on developing a full-fledged drive-through kiosk and food delivery system, utilizing LLMs to generate user-specific menus/deals.
//
// For more information:
// https://www.kutluhanaktar.com/projects/AI_driven_LoRa_LLM_enabled_Drive_through_Kiosk_Food_Delivery_System
//
//
// Connections
// RA-08H LoRaWan Node Board w/ RP2040 :
//                                Fermion 2.0" IPS TFT LCD Display (320x240)
// +5V     ------------------------ V
// GND     ------------------------ G
// 18/SCK  ------------------------ CK
// 19/MOSI ------------------------ SI
// 16/MISO ------------------------ SO
// 17/CS   ------------------------ CS
// 20      ------------------------ RT
// 21      ------------------------ DC
// -       ------------------------ BL
// 15      ------------------------ SC
//                                Crowtail - Serial Camera
// +5V     ------------------------ 5V
// GND     ------------------------ GND
// 8/TX1_H ------------------------ RX
// 9/RX1_H ------------------------ TX
//                                Custom KeyPad (4x4)
// 6       ------------------------ R1
// 7       ------------------------ R2
// 10      ------------------------ R3
// 11      ------------------------ R4
// 22      ------------------------ C1
// 23      ------------------------ C2
// 24      ------------------------ C3
// 25      ------------------------ C4
//                                5mm Common Anode RGB LED
// 12      ------------------------ R
// 13      ------------------------ G
// 14      ------------------------ B


// Include the required libraries:
#include <SPI.h>
#include <SD.h>
#include "DFRobot_GDL.h"
#include "DFRobot_Picdecoder_SD.h"
#include <Adafruit_VC0706.h>
#include <Keypad.h>

// Import the custom variables.
#include "custom_variables.h"

// Include the Edge Impulse FOMO model converted to an Arduino library and the necessary built-in Edge Impulse image (frame) processing functions.
#include <LoRa_LLM-enabled_Drive-through_Kiosk_Vehicle_Detection_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"

// Define the required parameters to run an inference with the provided Edge Impulse FOMO model.
#define CAPTURED_IMAGE_BUFFER_COLS    320
#define CAPTURED_IMAGE_BUFFER_ROWS    240
#define EI_CAMERA_FRAME_BYTE_SIZE     3
uint8_t *ei_camera_capture_out;

// Define the individual class names that of the provided Edge Impulse model.
int classes_item_num = 4;
String classes[] = {"a1dd", "a2f9", "a4c7", "a9f1"};

// Utilize the built-in MicroSD card reader on the Fermion 2.0" TFT LCD display (320x240).
#define SD_CS_PIN 15

// Define the Fermion TFT LCD display pin configurations.
#define TFT_DC  21
#define TFT_CS  17
#define TFT_RST 20

// Define the Fermion TFT LCD display object and integrated JPG decoder.
DFRobot_Picdecoder_SD decoder;
DFRobot_ST7789_240x320_HW_SPI screen(/*dc=*/TFT_DC,/*cs=*/TFT_CS,/*rst=*/TFT_RST);
#define SCREEN_WIDTH   240
#define SCREEN_HEIGHT  320

// Define the VC0706 camera object and assign the second hardware serial port for data transfer.
Adafruit_VC0706 serial_cam = Adafruit_VC0706(&Serial2);

// Define the custom keypad map — 4x4.
const byte ROWS = 4; // row_number
const byte COLS = 4; // column_number

// Define the custom keypad symbols with different configurations.
char num_Keys[ROWS][COLS] = {
  {'1','2','3','>'}, // [1, 2, 3, NEXT]
  {'4','5','6','<'}, // [4, 5, 6, PREVIOUS]
  {'7','8','9','='}, // [7, 8, 9, SELECT]
  {'+','0','-','!'}  // [ACTIVATE, 0, DELETE, EXIT]
};
char let_Keys[ROWS][COLS] = {
  {'a','b','c','>'}, // [A, B, C, NEXT]
  {'d','e','f','<'}, // [D, E, F, PREVIOUS]
  {'x','x','x','='}, // [X, X, X, SELECT]
  {'+','x','-','!'}  // [ACTIVATE, X, DELETE, EXIT]
};

// Define the custom keypad row and column pins.
byte rowPins[ROWS] = {6, 7, 10, 11};
byte colPins[COLS] = {22, 23, 24, 25};

// Initialize the custom keypad maps with the given settings — numbers and letters.
Keypad num_keypad = Keypad(makeKeymap(num_Keys), rowPins, colPins, ROWS, COLS);
Keypad let_keypad = Keypad(makeKeymap(let_Keys), rowPins, colPins, ROWS, COLS);

// Define the RGB LED pins.
#define red_pin           12
#define green_pin         13
#define blue_pin          14

// Define the required RA-08H module configurations and the associated TTN server application settings by creating a struct — ttn_settings.
struct ttn_settings{
  String DEVEUI = "TTN_DEVEUI"; // e.g., 60B3C58ED016FF89
  String APPEUI = "TTN_APPEUI"; // e.g., 1111111111111111
  String APPKEY = "TTN_APPKEY"; // e.g., 39C4BC565188ABE36AFC123754C42FB0
  boolean ra_08h_update_init = true;
  boolean ra_08h_update_ongoing = false;
  int _delay = 500;
  char con_status[8] = {'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w'};
};

// Define the data holders:
struct ttn_settings ttn_settings;
volatile boolean scr_init = true;
volatile boolean active_int_opt[4] = {false, false, false, false};
int selected_interface_opt = 0;
String order_menu_command = "";
String account_auth_key = "";
int current_keypad_map = 1; // 1: Numbers, 2: Letters
volatile boolean vehicle_validation_status = false;
const int IMG_SAMPLE_NUM = 5;
char img_save_status[IMG_SAMPLE_NUM] = {'w', 'w', 'w', 'w', 'w'};
char inference_status = 'i'; // i [IDLE], n [NOT DETECTED], d [DETECTED]
String detected_class = "";


void setup(){
  Serial.begin(9600);

  // Initiate the first hardware serial port with its default RX and TX pins to communicate with the onboard RA-08H.
  Serial1.setRX(1); Serial1.setTX(0);
  Serial1.begin(9600);

  // Initiate the second hardware serial port with its default RX and TX pins to communicate with the Crowtail serial camera.  
  Serial2.setRX(9); Serial2.setTX(8);

  // Register pin configurations.
  pinMode(red_pin, OUTPUT); pinMode(green_pin, OUTPUT); pinMode(blue_pin, OUTPUT);
  adjustColor(0,0,0);

  // Initialize the Fermion TFT LCD display. 
  screen.begin();
  screen.setRotation(4);
  delay(1000);
  
  // Initialize the MicroSD card module on the Fermion TFT LCD display.
  while(!SD.begin(SD_CS_PIN)){
    Serial.println("SD Card => No module found!");
    delay(200);
    return;
  }
   
  // Initialize the Crowtail serial camera (VC0706).
  if(serial_cam.begin()){
    Serial.println("Crowtail serial camera is working successfully!");
  }else{
    Serial.println("Crowtail serial camera is not working!");
    return;
  }

  // Set the serial camera picture size: VC0706_640x480, VC0706_320x240, or VC0706_160x120.
  serial_cam.setImageSize(VC0706_320x240);

}

void loop(){
  // Initiate the welcome screen.
  if(scr_init){
    show_screen("init", selected_interface_opt, scr_init);
  } scr_init = false;

  // Get the pressed keycap value (number).
  char activeKey = num_keypad.getKey();
  if(activeKey != NO_KEY){
    // Update the highlighted interface option if the '>' (NEXT) or '<' (PREVIOUS) keycaps are pressed.
    if(activeKey == '>'){
      selected_interface_opt--;
      if(selected_interface_opt < 0) selected_interface_opt = 4;
      show_screen("init", selected_interface_opt, false);
    }     
    if(activeKey == '<'){
      selected_interface_opt++;
      if(selected_interface_opt > 4) selected_interface_opt = 0;
      show_screen("init", selected_interface_opt, false);
    }
    // Select the highlighted interface option if the '=' (SELECT) keycap is pressed.
    if(activeKey == '=' && selected_interface_opt > 0){
      active_int_opt[selected_interface_opt-1] = true;
    }  
  }

  // Display the selected interface option.
  if(active_int_opt[0]){
    show_screen("order_menu", selected_interface_opt, true);
    while(active_int_opt[0]){
      // Get the pressed keycap value depending on the selected keypad map — numbers and letters..
      char activeKey = (current_keypad_map == 1) ? num_keypad.getKey() : let_keypad.getKey();
      if(activeKey != NO_KEY){
        switch(activeKey){
          case '+':
            // Transfer the entered order menu command through the established LoRaWAN network if the '+' (ACTIVATE) keycap is pressed.
            if(order_menu_command.length() == 4){
              adjustColor(0,255,255);
              bool send_status = ra_08h_send_to_lora_gateway(order_menu_command);
              // Notify the user of the LoRa data transfer success on the screen.
              if(send_status){ show_screen("order_menu_success", selected_interface_opt, true); }
              else{ show_screen("order_menu_failure", selected_interface_opt, true); }
              // Clear the order menu command after sending it.
              order_menu_command = "";
            }
          break;
          case '>':
            // Change the keypad to numbers if the '>' (NEXT) keycap is pressed.
            current_keypad_map = 1;
            // Notify the user of the map change success via the screen.
            show_screen("order_menu", selected_interface_opt, false);
          break;
          case '<':
            // Change the keypad to letters if the '<' (PREVIOUS) keycap is pressed.
            current_keypad_map = 2;
            // Notify the user of the map change success via the screen.
            show_screen("order_menu", selected_interface_opt, false);
          break;          
          case '-':
            // Discard one character from the order menu string if the '-' (DELETE) keycap is pressed.
            if(order_menu_command.length() > 0){
              order_menu_command.remove(order_menu_command.length()-1);
              // Show the modified string on the screen.
              show_screen("order_menu", selected_interface_opt, false);
              delay(150);              
            }
          break;
          case '!':
            // Return to the main interface menu if the '!' (EXIT) keycap is pressed.
            active_int_opt[0] = false;
            scr_init = true;
          break;
          case '=':
            // Do nothing if the '=' (SELECT) keycap is pressed.
          break;
          case 'x':
            // Do nothing if an unassigned letter 'x' (X) keycap is pressed.
          break;          
          default:
            // Add the pressed keycap (number or letter) to the order menu string to generate the LoRa command.
            if(order_menu_command.length() < 4){
              order_menu_command += String(activeKey);
              // Show the modified string on the screen.
              show_screen("order_menu", selected_interface_opt, false);
              delay(250);
            }
          break;
        }
      }
    }
  }

  if(active_int_opt[1]){
    show_screen("valid_vehicle", selected_interface_opt, true);
    while(active_int_opt[1]){
      // Get the pressed keycap value depending on the selected keypad map — numbers and letters..
      char activeKey = (current_keypad_map == 1) ? num_keypad.getKey() : let_keypad.getKey();
      if(activeKey != NO_KEY){
        switch(activeKey){
          case '+':
            // If the user did not authorize a registered vehicle yet, enable AI-based vehicle validation.
            if(!vehicle_validation_status){          
              // Notify the user as the model is activated on the screen.
              show_screen("valid_vehicle_run", selected_interface_opt, true);          
              // Run inference with the object detection model to authenticate the registered vehicle.
              run_inference();
              // Transfer the detected vehicle class (code) through the established LoRaWAN network.
              if(inference_status == 'd'){
                bool send_status = ra_08h_send_to_lora_gateway(detected_class);
                // According to the LoRa data transfer success, update the vehicle validation status.
                vehicle_validation_status = (send_status) ? true : false;
                delay(1000);
                // Inform the user of the model detection and vehicle validation (LoRa) results on the screen.
                show_screen("valid_vehicle_run", selected_interface_opt, false);
              }else{
                // Inform the user of the model detection results (not found) on the screen.
                show_screen("valid_vehicle_run", selected_interface_opt, false);
              }
            }else{
              // Otherwise, let the user discard the validated vehicle from the kiosk.
              vehicle_validation_status = false;
              detected_class = "";
              show_screen("valid_vehicle", selected_interface_opt, true);
            }
          break;
          case '!':
            // Return to the main interface menu if the '!' (EXIT) keycap is pressed.
            active_int_opt[1] = false;
            scr_init = true;
            // Return the model inference status to default (IDLE).
            inference_status = 'i';
          break;
          default:
            // Do nothing is a keycap without an assigned task is pressed.
          break;
        }
      }
    }
  }    

  if(active_int_opt[2]){
    show_screen("add_vehicle", selected_interface_opt, true);
    // Clear the image sample status is previously registered.
    for(int i = 0; i < IMG_SAMPLE_NUM; i++){
      img_save_status[i] = 'w';
    }
    while(active_int_opt[2]){
      // Get the pressed keycap value depending on the selected keypad map — numbers and letters.
      char activeKey = (current_keypad_map == 1) ? num_keypad.getKey() : let_keypad.getKey();
      if(activeKey != NO_KEY){
        // Enable adding a new vehicle if the user did not authorize a registered vehicle.
        if(!vehicle_validation_status){
          switch(activeKey){
            case '+':
              // Obtain the account authentication key (generated by the web dashboard) if the '+' (ACTIVATE) keycap is pressed.
              if(account_auth_key.length() == 4){
                // Activate the image sample collection procedure for the new vehicle.
                show_screen("add_vehicle_img_collect", selected_interface_opt, true);
                delay(500);
                // Capture and save the required vehicle image samples — 5 — for further model training. 
                for(int i = 0; i < IMG_SAMPLE_NUM; i++){
                  capture_and_save_picture(account_auth_key, i);
                  // Notify the user of the status of the image capturing process.
                  show_screen("add_vehicle_img_collect", selected_interface_opt, false);
                  delay(1000);
                }
                // Clear the account authentication key after saving image samples.
                account_auth_key = "";
              }
            break;
            case '>':
              // Change the keypad to numbers if the '>' (NEXT) keycap is pressed.
              current_keypad_map = 1;
              // Notify the user of the map change success via the screen.
              show_screen("add_vehicle", selected_interface_opt, false);
            break;
            case '<':
              // Change the keypad to letters if the '<' (PREVIOUS) keycap is pressed.
              current_keypad_map = 2;
              // Notify the user of the map change success via the screen.
              show_screen("add_vehicle", selected_interface_opt, false);
            break;          
            case '-':
              // Discard one character from the authentication key string if the '-' (DELETE) keycap is pressed.
              if(account_auth_key.length() > 0){
                account_auth_key.remove(account_auth_key.length()-1);
                // Show the modified string on the screen.
                show_screen("add_vehicle", selected_interface_opt, false);
                delay(150);              
              }
            break;
            case '!':
              // Return to the main interface menu if the '!' (EXIT) keycap is pressed.
              active_int_opt[2] = false;
              scr_init = true;
            break;
            case '=':
              // Do nothing if the '=' (SELECT) keycap is pressed.
            break;
            case 'x':
              // Do nothing if an unassigned letter 'x' (X) keycap is pressed.
            break;          
            default:
              // Add the pressed keycap (number or letter) to the authentication key string.
              if(account_auth_key.length() < 4){
                account_auth_key += String(activeKey);
                // Show the modified string on the screen.
                show_screen("add_vehicle", selected_interface_opt, false);
                delay(250);
              }
            break;
          }
        }else{
          switch(activeKey){
            case '!':
              // Return to the main interface menu if the '!' (EXIT) keycap is pressed.
              active_int_opt[2] = false;
              scr_init = true;
            break;
            default:
              // Do nothing is a keycap without an assigned task is pressed.
            break;                        
          }
        }
      }
    }
  }  

  if(active_int_opt[3]){
    show_screen("reset_con", selected_interface_opt, true);
    while(active_int_opt[3]){
      // Get the pressed keycap value depending on the selected keypad map — numbers and letters..
      char activeKey = (current_keypad_map == 1) ? num_keypad.getKey() : let_keypad.getKey();
      if(activeKey != NO_KEY){
        switch(activeKey){
          case '+':
            // If requested, configure the RA-08H LoRa module connection settings based on the provided TTN server application.
            if(ttn_settings.ra_08h_update_init){
              configure_ra_08h_lora_settings(ttn_settings.DEVEUI, ttn_settings.APPEUI, ttn_settings.APPKEY);
              // Notify the user of the LoRa settings update status on the screen.
              show_screen("reset_con", selected_interface_opt, false);
            } 
          break;
          case '!':
            // Return to the main interface menu if the '!' (EXIT) keycap is pressed.
            active_int_opt[3] = false;
            scr_init = true;
          break;
          default:
            // Do nothing is a keycap without an assigned task is pressed.
          break;
        }
      }
    }
  }  

}

void run_inference(){
  adjustColor(0,0,255);
  // Summarize the Edge Impulse FOMO model inference settings (from model_metadata.h).
  ei_printf("\nInference settings:\n");
  ei_printf("\tImage resolution: %dx%d\n", EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
  ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
  ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

  // Capture a new image (frame) via the Crowtail serial camera (VC0706).
  if(serial_cam.takePicture()){
    // Convert the captured raw RGB565 image buffer to a RGB888 buffer required by the Edge Impulse functions.
    ei_camera_capture_out = (uint8_t*)malloc(CAPTURED_IMAGE_BUFFER_COLS * CAPTURED_IMAGE_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
    RBG565ToRGB888(serial_cam.readPicture(serial_cam.frameLength()), ei_camera_capture_out, serial_cam.frameLength());
    // Depending on the given model's image (frame) resolution, resize the converted RGB888 buffer by utilizing built-in Edge Impulse functions.
    if(CAPTURED_IMAGE_BUFFER_COLS != EI_CLASSIFIER_INPUT_WIDTH && CAPTURED_IMAGE_BUFFER_ROWS != EI_CLASSIFIER_INPUT_HEIGHT){
      ei::image::processing::crop_and_interpolate_rgb888(
        ei_camera_capture_out, // Output image buffer, can be same as the input image buffer.
        CAPTURED_IMAGE_BUFFER_COLS,
        CAPTURED_IMAGE_BUFFER_ROWS,
        ei_camera_capture_out,
        EI_CLASSIFIER_INPUT_WIDTH,
        EI_CLASSIFIER_INPUT_HEIGHT);
    }
    // Run an inference to make predictions based on the trained classes.
    ei::signal_t signal;
    // Create a signal object from the converted and resized RGB888 image buffer.
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_cutout_get_data;
    // Run the provided classifier.
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR _err = run_classifier(&signal, &result, false);
    if(_err != EI_IMPULSE_OK){
      ei_printf("ERR: Failed to run classifier (%d)\n", _err);
      return;
    }

    // Print the inference timings on the serial monitor.
    ei_printf("\nPredictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);

    // Obtain the object detection results and bounding boxes for the detected labels (classes). 
    bool bb_found = result.bounding_boxes[0].value > 0;
    for(size_t ix = 0; ix < EI_CLASSIFIER_OBJECT_DETECTION_COUNT; ix++){
      auto bb = result.bounding_boxes[ix];
      if(bb.value == 0) continue;
      // Print the calculated bounding box measurements on the serial monitor.
      ei_printf("    %s (", bb.label);
      ei_printf_float(bb.value);
      ei_printf(") [ x: %u, y: %u, width: %u, height: %u ]\n", bb.x, bb.y, bb.width, bb.height);
      // Fetch the predicted label (class) and the detected object's bounding box measurements (if necessary).
      for(int i=0; i<classes_item_num; i++){
        if(bb.label == classes[i]){
          detected_class = bb.label;
          inference_status = 'd';
          ei_printf("\nPredicted Class [Label]: %s\n", detected_class); 
        }
      }
    }
    // Notify the user the model cannot detect any labels.
    if(!bb_found){
      ei_printf("\nPredicted Class [Label]: No objects found!\n");
      detected_class = "none";
      inference_status = 'n';
    }
    // Notify the user of the model anomalies if occured any while running the inference.
    #if EI_CLASSIFIER_HAS_ANOMALY == 1
      ei_printf("Anomaly: ");
      ei_printf_float(result.anomaly);
      ei_printf("\n");
    #endif 
    // Release the image buffer processed by the signal object.
    ei_free(ei_camera_capture_out);
    // Reset the Crowtail serial camera (VC0706) after running the inference to clear the image frame.
    serial_cam.reset();    
  }
}

bool RBG565ToRGB888(uint8_t *src_buf, uint8_t *dst_buf, uint32_t src_len){
    uint8_t hb, lb;
    uint32_t pix_count = src_len / 2;
    // Convert the passed raw RGB565 image buffer to an RGB888 buffer.
    for(uint32_t i = 0; i < pix_count; i ++) {
       // Next source byte.
        hb = *src_buf++;
        lb = *src_buf++;
        // Conversion [RGB565 -> RGB888].
        *dst_buf++ = hb & 0xF8;
        *dst_buf++ = (hb & 0x07) << 5 | (lb & 0xE0) >> 3;
        *dst_buf++ = (lb & 0x1F) << 3;
    }
    return true;
}

static int ei_camera_cutout_get_data(size_t offset, size_t length, float *out_ptr){
  // Convert the given image data (buffer) to the out_ptr format required by the Edge Impulse FOMO model.
  size_t pixel_ix = offset * 3;
  size_t pixels_left = length;
  size_t out_ptr_ix = 0;
  // Since the image data is already converted to an RGB888 buffer, directly recalculate offset into pixel index.
  while(pixels_left != 0){  
    out_ptr[out_ptr_ix] = (ei_camera_capture_out[pixel_ix] << 16) + (ei_camera_capture_out[pixel_ix + 1] << 8) + ei_camera_capture_out[pixel_ix + 2];
    // Move to the next pixel.
    out_ptr_ix++;
    pixel_ix+=3;
    pixels_left--;
  }
  return 0;
}

bool ra_08h_send_to_lora_gateway(String data_packet){
  String data_res = "";
  // Send the passed data packet to the connected LR1302 LoRaWAN Gateway Module.
  Serial1.print("AT+DTRX=1,2,3," + data_packet + "\n"); delay(ttn_settings._delay); 
  ttn_settings.ra_08h_update_ongoing = true; data_res = ra_08h_update_response(); Serial.println(data_res);
  // Notify the user of the LoRa data packet transfer success.
  if(data_res.indexOf("ERR") >= 0){ adjustColor(255,0,0); return false; }
  else{ adjustColor(0,255,0); return true; }
}

void configure_ra_08h_lora_settings(String DEVEUI, String APPEUI, String APPKEY){
  String update_res = "";
  Serial.println("RA-08H LoRa Setting Configuration Started...\n");
  adjustColor(255,0,255);
  /*
    Update the RA-08H settings via AT commands according to the generated TTN server application — DEVEUI, JOINEUI (formerly called APPEUI), and APPKEY.
    After obtaining the response from the RA-08H LoRa module, change the update status of the associated command accordingly. 
  */
  // Set the node access method to OTAA
  Serial1.print("AT+CJOINMODE=0\n"); delay(ttn_settings._delay); 
  ttn_settings.ra_08h_update_ongoing = true; update_res = ra_08h_update_response(); Serial.println(update_res);
  ttn_settings.con_status[0] = (update_res.indexOf("ERR") >= 0 || update_res.indexOf("FAIL") >= 0) ? 'f' : 's';
  // Set the node group frequency mask (Set channels 0-7)
  Serial1.print("AT+CFREQBANDMASK=0002\n"); delay(ttn_settings._delay); 
  ttn_settings.ra_08h_update_ongoing = true; update_res = ra_08h_update_response(); Serial.println(update_res);
  ttn_settings.con_status[1] = (update_res.indexOf("ERR") >= 0 || update_res.indexOf("FAIL") >= 0) ? 'f' : 's';
  // Set the node type
  Serial1.print("AT+CCLASS=2\n"); delay(ttn_settings._delay); 
  ttn_settings.ra_08h_update_ongoing = true; update_res = ra_08h_update_response(); Serial.println(update_res);
  ttn_settings.con_status[2] = (update_res.indexOf("ERR") >= 0 || update_res.indexOf("FAIL") >= 0) ? 'f' : 's';
  // Assign the associated TTN application end device DEVEUI
  Serial1.print("AT+CDEVEUI=" + DEVEUI + "\n"); delay(ttn_settings._delay); 
  ttn_settings.ra_08h_update_ongoing = true; update_res = ra_08h_update_response(); Serial.println(update_res);
  ttn_settings.con_status[3] = (update_res.indexOf("ERR") >= 0 || update_res.indexOf("FAIL") >= 0) ? 'f' : 's';
  // Assign the associated TTN application end device JOINEUI (formerly called APPEUI)
  Serial1.print("AT+CAPPEUI=" + APPEUI + "\n"); delay(ttn_settings._delay); 
  ttn_settings.ra_08h_update_ongoing = true; update_res = ra_08h_update_response(); Serial.println(update_res);
  ttn_settings.con_status[4] = (update_res.indexOf("ERR") >= 0 || update_res.indexOf("FAIL") >= 0) ? 'f' : 's';
  // Assign the associated TTN application end device APPKEY
  Serial1.print("AT+CAPPKEY=" + APPKEY + "\n"); delay(ttn_settings._delay); 
  ttn_settings.ra_08h_update_ongoing = true; update_res = ra_08h_update_response(); Serial.println(update_res);
  ttn_settings.con_status[5] = (update_res.indexOf("ERR") >= 0 || update_res.indexOf("FAIL") >= 0) ? 'f' : 's';
  // Set the node uplink and downlink frequency — 1 for same frequency, 2 for different frequency 
  Serial1.print("AT+CULDLMODE=2\n"); delay(ttn_settings._delay);  
  ttn_settings.ra_08h_update_ongoing = true; update_res = ra_08h_update_response(); Serial.println(update_res);
  ttn_settings.con_status[6] = (update_res.indexOf("ERR") >= 0 || update_res.indexOf("FAIL") >= 0) ? 'f' : 's';
  // Set the node to join the network
  Serial1.print("AT+CJOIN=1,1,10,3\n"); delay(ttn_settings._delay); 
  ttn_settings.ra_08h_update_ongoing = true; update_res = ra_08h_update_response(); Serial.println(update_res);
  ttn_settings.con_status[7] = (update_res.indexOf("ERR") >= 0 || update_res.indexOf("FAIL") >= 0) ? 'f' : 's';
  // Update completed.
  Serial.println("RA-08H LoRa Setting Configuration Ended...\n");
  adjustColor(0,255,0);
}

String ra_08h_update_response(){
  String ra_08h_response = "No response!";
  int port_wait = 0;
  // Wait until RA-08H transfers a response after sending AT commands via serial communication.
  while(ttn_settings.ra_08h_update_ongoing){
    port_wait++;
    if(Serial1.available()){
      ra_08h_response = Serial1.readString();
    }
    // Halt the loop if the RA-08H returns a data packet or does not respond in the given timeframe.
    if(ra_08h_response != "" || port_wait > 30000){
      ttn_settings.ra_08h_update_ongoing = false;
    }
  }
  // Then, return the retrieved response.
  delay(500);
  return ra_08h_response;
}

void capture_and_save_picture(String account_key, int sample_num){
  adjustColor(255,255,0);
  if(serial_cam.takePicture()){
    // If the Crowtail serial camera (VC0706) successfully captured a frame (image):
    Serial.println("Crowtail => Image captured successfully!");
    // After capturing the frame, obtain the length of the image buffer.
    uint32_t buf_len = serial_cam.frameLength();
    // Then, save the image buffer to the SD card with the passed account authentication key and the image sample number — in the JPG format.
    String filename = "vehicle_image_samples/" + account_key + "__" + String(sample_num+1) + ".jpg";
    File new_img_file = SD.open(filename.c_str(), "w");
    // According to the obtained image buffer length (total byte number), write the retrieved buffer to the new image file 32 bytes at a time.
    byte byte_n = 0;
    while(buf_len > 0){
      uint8_t *img_buffer;
      uint8_t currentBytes = min((uint32_t)32, buf_len);
      img_buffer = serial_cam.readPicture(currentBytes);
      new_img_file.write(img_buffer, currentBytes);
      if(++byte_n >= 64){ Serial.print('.'); byte_n = 0; }
      buf_len -= currentBytes;
    } new_img_file.close();
    // Capture success.
    img_save_status[sample_num] = 's';
    Serial.println("Crowtail => Image saved to the SD card successfully!");
    adjustColor(0,255,0);    
  }else{
    // Capture failure.
    img_save_status[sample_num] = 'f';
    Serial.println("Crowtail => Cannot capture image!");
    adjustColor(255,0,0);
  }
  // Reset the Crowtail serial camera (VC0706) after taking a snaphot to clear the image frame.
  serial_cam.reset();
}

void show_screen(String _type, int _opt, volatile boolean _clear){
  // According to the given parameters, show the requested screen type on the Fermion TFT LCD display.
  int pic_w = 60, pic_h = 60, pic_f = 8;
  int h_y_s = 25;
  int text_s_s = 6, text_m_s = 12, text_b_s = 18, text_bb_s = 24;
  if(_type == "init"){
    adjustColor(0,0,0);
    if(_clear) screen.fillScreen(c_green_1);
    screen.fillRoundRect(0, h_y_s, pic_w+pic_f, pic_h+pic_f, pic_f/2, c_orange_3);
    decoder.drawPicture("assets/lora_icon.bmp", pic_f/2, h_y_s+pic_f/2, pic_w+pic_f/2, h_y_s+pic_h+pic_f/2, screenDrawPixel);
    screen.setTextColor(c_orange_1);
    screen.setTextSize(3);
    screen.setCursor(pic_w+pic_f+11, h_y_s+pic_f);
    screen.print("LoRa");
    screen.setCursor(pic_w+pic_f+11, h_y_s+pic_h+pic_f-22);
    screen.print("Kiosk");
    screen.fillRoundRect(SCREEN_WIDTH-(pic_w+pic_f), h_y_s, pic_w+pic_f, pic_h+pic_f, pic_f/2, c_orange_3);
    String opt_img = "assets/menu_logo_" + String(_opt) + ".bmp";
    decoder.drawPicture(opt_img.c_str(), (SCREEN_WIDTH-(pic_w+pic_f))+pic_f/2, h_y_s+pic_f/2, (SCREEN_WIDTH-(pic_w+pic_f))+pic_w+pic_f/2, h_y_s+pic_h+pic_f/2, screenDrawPixel);
    int y_end = h_y_s+pic_h+pic_f, x_str = 32, init_sp = 45, l_sp = 40;
    int c_r = 10, c_x_s = x_str - c_r - 10, c_r_sp = 4, c_l_sp = 5;
    screen.setTextSize(2);
    int highlight_c = (_opt == 1) ? c_blue_4 : c_blue_3;
    int border_c = c_orange_1;
    y_end += init_sp; screen.fillCircle(c_x_s, y_end+c_l_sp, c_r, border_c); screen.fillCircle(c_x_s, y_end+c_l_sp, c_r-c_r_sp, highlight_c);
    screen.setCursor(x_str, y_end);
    screen.setTextColor(highlight_c);
    screen.print("Order Menu");
    highlight_c = (_opt == 2) ? c_blue_4 : c_blue_3;
    y_end += l_sp; screen.fillCircle(c_x_s, y_end+c_l_sp, c_r, border_c); screen.fillCircle(c_x_s, y_end+c_l_sp, c_r-c_r_sp, highlight_c);
    screen.setCursor(x_str, y_end);
    screen.setTextColor(highlight_c);
    screen.print("Validate Vehicle");    
    highlight_c = (_opt == 3) ? c_blue_4 : c_blue_3;
    y_end += l_sp; screen.fillCircle(c_x_s, y_end+c_l_sp, c_r, border_c); screen.fillCircle(c_x_s, y_end+c_l_sp, c_r-c_r_sp, highlight_c);
    screen.setCursor(x_str, y_end);
    screen.setTextColor(highlight_c);
    screen.print("Add New Vehicle"); 
    highlight_c = (_opt == 4) ? c_blue_4 : c_blue_3;
    y_end += l_sp; screen.fillCircle(c_x_s, y_end+c_l_sp, c_r, border_c); screen.fillCircle(c_x_s, y_end+c_l_sp, c_r-c_r_sp, highlight_c);
    screen.setCursor(x_str, y_end);
    screen.setTextColor(highlight_c);
    screen.print("Reset Connection"); 
  }
  
  else if(_type == "order_menu"){  
    if(_clear) screen.fillScreen(c_green_1);
    screen.fillRoundRect(0, h_y_s, pic_w+pic_f, pic_h+pic_f, pic_f/2, c_orange_3);
    String opt_img = "assets/menu_logo_" + String(_opt) + ".bmp";
    decoder.drawPicture(opt_img.c_str(), pic_f/2, h_y_s+pic_f/2, pic_w+pic_f/2, h_y_s+pic_h+pic_f/2, screenDrawPixel);
    screen.setTextColor(c_orange_1);
    screen.setTextSize(3);
    screen.setCursor(pic_w+pic_f+11, h_y_s+pic_f);
    screen.print("Order");
    screen.setCursor(pic_w+pic_f+11, h_y_s+pic_h+pic_f-22);
    screen.print("Menu");
    int y_end = h_y_s+pic_h+pic_f, init_sp = 55;
    int tri_x_sp = 5, tri_w = 20, tri_h = 30, tri_sc = 4;
    int x_str = tri_x_sp + tri_w + 10;
    y_end += init_sp;
    screen.setTextColor(c_white_1);
    screen.setTextSize(2);
    screen.fillTriangle(tri_x_sp, y_end-(tri_h/2), tri_x_sp+tri_w, y_end, tri_x_sp, y_end+(tri_h/2), c_green_3);
    screen.fillTriangle(tri_x_sp+(tri_sc/2), y_end-(tri_h/2)+tri_sc, tri_x_sp+tri_w-tri_sc, y_end, tri_x_sp+(tri_sc/2), y_end+(tri_h/2)-tri_sc, c_white_1);    
    screen.setCursor(x_str, y_end-(text_m_s/2));
    screen.print("Enter Menu Code");
    int l_str = (SCREEN_WIDTH - (text_bb_s * order_menu_command.length())) / 2;
    y_end += (tri_h/2) + (text_bb_s*1.5);
    screen.fillRect(0, y_end-5, SCREEN_WIDTH, text_bb_s+10, c_green_1);
    int _hightlight_c = (order_menu_command.length() == 4) ? c_green_3 : c_white_1;
    screen.setTextColor(_hightlight_c);
    screen.setTextSize(4);
    screen.setCursor(l_str, y_end);
    screen.print(order_menu_command);
    int f_text_sp = 5;
    y_end = SCREEN_HEIGHT - f_text_sp - (text_s_s*2);
    screen.fillRect(0, y_end-5, text_s_s*8, (text_s_s*2)+f_text_sp, c_green_1);
    screen.setTextSize(1);
    screen.setTextColor(c_orange_2);
    screen.setCursor(f_text_sp, y_end);
    String cur_keypad = (current_keypad_map == 1) ? "K: 123" : "K: ABC";
    screen.print(cur_keypad);
    String auth_status = (vehicle_validation_status) ? "VEHICLE: AUTHORIZED" : "VEHICLE: NO_AUTH";
    int auth_color = (vehicle_validation_status) ? c_green_3 : c_red_1;
    int f_text_right_x_str = SCREEN_WIDTH - (auth_status.length()*text_s_s) - f_text_sp;
    screen.setTextColor(auth_color);
    screen.setCursor(f_text_right_x_str, y_end);
    screen.print(auth_status);
  }

  else if(_type == "order_menu_success"){
    int b_pic_w = pic_w*2, b_pic_h = pic_h*2, b_pic_f = pic_f*2, b_pic_y_sp = 15;
    int y_sp = 15, y_l_sp = 30;
    int y_end = y_sp;
    if(_clear) screen.fillScreen(c_green_1);
    screen.setCursor(0, y_end);
    screen.setTextColor(c_green_3);
    screen.setTextSize(3);
    screen.print("Menu: " + order_menu_command);
    y_end += text_bb_s + y_l_sp;
    screen.setCursor(0, y_end);
    screen.setTextColor(c_green_5);
    screen.setTextSize(1);
    screen.println("The LoRa code of the requested menu has been successfully transferred \nvia the established LoRaWAN network.\n");
    screen.print("Please inspect your web dashboard to \ntrack the order status and receive the \nrequested menu / deal. The payment \nprocess is handled via the provided \naccount settings.");
    y_end = SCREEN_HEIGHT - b_pic_y_sp - b_pic_h - b_pic_f;
    int b_pic_x_str = (SCREEN_WIDTH/2) - ((b_pic_w+b_pic_f)/2);
    screen.fillRoundRect(b_pic_x_str, y_end, b_pic_w+b_pic_f, b_pic_w+b_pic_f, b_pic_f/2, c_green_4);
    decoder.drawPicture("assets/order_success.bmp", b_pic_x_str+(b_pic_f/2), y_end+(b_pic_f/2), b_pic_x_str+(b_pic_f/2)+b_pic_w, y_end+(b_pic_f/2)+b_pic_h, screenDrawPixel);    
  }

  else if(_type == "order_menu_failure"){
    int b_pic_w = pic_w*2, b_pic_h = pic_h*2, b_pic_f = pic_f*2, b_pic_y_sp = 15;
    int y_sp = 15, y_l_sp = 30;
    int y_end = y_sp;
    if(_clear) screen.fillScreen(c_green_1);
    screen.setCursor(0, y_end);
    screen.setTextColor(c_red_2);
    screen.setTextSize(3);
    screen.print("Menu: " + order_menu_command);
    y_end += text_bb_s + y_l_sp;
    screen.setCursor(0, y_end);
    screen.setTextColor(c_red_1);
    screen.setTextSize(1);
    screen.println("Cannot send the given code via the \nLoRaWAN network.\n\nPlease reset the module connection\nsettings via the user interface.");
    y_end = SCREEN_HEIGHT - b_pic_y_sp - b_pic_h - b_pic_f;
    int b_pic_x_str = (SCREEN_WIDTH/2) - ((b_pic_w+b_pic_f)/2);
    screen.fillRoundRect(b_pic_x_str, y_end, b_pic_w+b_pic_f, b_pic_w+b_pic_f, b_pic_f/2, c_red_3);
    decoder.drawPicture("assets/order_failure.bmp", b_pic_x_str+(b_pic_f/2), y_end+(b_pic_f/2), b_pic_x_str+(b_pic_f/2)+b_pic_w, y_end+(b_pic_f/2)+b_pic_h, screenDrawPixel);     
  }

  else if(_type == "valid_vehicle"){
    int _pic_w = 120, _pic_h = 120, _pic_f = 15, _pic_y_sp = 15;
    if(_clear) screen.fillScreen(c_green_1);
    screen.fillRoundRect(0, h_y_s, pic_w+pic_f, pic_h+pic_f, pic_f/2, c_orange_3);
    String opt_img = "assets/menu_logo_" + String(_opt) + ".bmp";
    decoder.drawPicture(opt_img.c_str(), pic_f/2, h_y_s+pic_f/2, pic_w+pic_f/2, h_y_s+pic_h+pic_f/2, screenDrawPixel);
    screen.setTextColor(c_orange_1);
    screen.setTextSize(3);
    screen.setCursor(pic_w+pic_f+11, h_y_s+pic_f);
    screen.print("Validate");
    screen.setCursor(pic_w+pic_f+11, h_y_s+pic_h+pic_f-22);
    screen.print("Vehicle");
    int y_end = h_y_s+pic_h+pic_f, init_sp = 35;
    y_end += init_sp;
    String highlight_mes, highlight_pic; int highlight_c;
    if(vehicle_validation_status){
      highlight_c = c_edge_g;
      highlight_mes = "Already validated your registered\nvehicle; thanks for your patronage :)\nClick again to discard validation!";
      highlight_pic = "assets/run_inference_success.bmp";      
    }else{
      highlight_c = c_edge_b;
      highlight_mes = "To order menus and campaigns\nspecifically generated for you, please\nauthorize your registered vehicle!";
      highlight_pic = "assets/run_inference_idle.bmp";
    }
    screen.setTextColor(highlight_c);
    screen.setTextSize(1);
    screen.setCursor(0, y_end);
    screen.print(highlight_mes);
    y_end = SCREEN_HEIGHT - _pic_h - _pic_f/2 - _pic_y_sp;
    int _pic_x_str = (SCREEN_WIDTH - (_pic_w + _pic_f)) / 2;
    screen.fillRoundRect(_pic_x_str, y_end, _pic_w+_pic_f, _pic_h+_pic_f, _pic_f/2, highlight_c);
    decoder.drawPicture(highlight_pic.c_str(), _pic_x_str+(_pic_f/2), y_end+(_pic_f/2), _pic_x_str+(_pic_f/2)+_pic_w, y_end+(_pic_f/2)+_pic_h, screenDrawPixel);
  }
  
  else if(_type == "valid_vehicle_run"){
    int _pic_w = 80, _pic_h = 80, _pic_f = 10, _pic_x_sp = 5, _pic_y_sp = 35;
    int y_end = _pic_y_sp;
    int _pic_x_str = _pic_x_sp;
    int hor_line_w = SCREEN_WIDTH - _pic_w  - _pic_f, hor_line_h = _pic_f/2;
    int hor_line_x_str = _pic_x_str + (_pic_f/2) + (_pic_w/2), hor_line_y_str = y_end + (_pic_f/2) + (_pic_h/2) - (hor_line_h/2);
    int highlight_c = (inference_status == 'i') ? c_edge_y : ((inference_status == 'n') ? c_edge_r : c_edge_g);
    String highlight_mes = (inference_status == 'i') ? "Running..." : ((inference_status == 'n') ? "Not registered!" : "Vehicle Code: " + detected_class);
    if(_clear) screen.fillScreen(c_green_1);
    screen.fillRect(hor_line_x_str, hor_line_y_str, hor_line_w, hor_line_h, highlight_c);
    screen.fillRoundRect(_pic_x_str, y_end, _pic_w+_pic_f, _pic_h+_pic_f, _pic_f/2, highlight_c);
    decoder.drawPicture("assets/edge_impulse_icon.bmp", _pic_x_str+(_pic_f/2), y_end+(_pic_f/2), _pic_x_str+(_pic_f/2)+_pic_w, y_end+(_pic_f/2)+_pic_h, screenDrawPixel);
    _pic_x_str = SCREEN_WIDTH - _pic_f - _pic_w - _pic_x_sp;
    screen.fillRoundRect(_pic_x_str, y_end, _pic_w+_pic_f, _pic_h+_pic_f, _pic_f/2, highlight_c);
    decoder.drawPicture("assets/run_inference_cam.bmp", _pic_x_str+(_pic_f/2), y_end+(_pic_f/2), _pic_x_str+(_pic_f/2)+_pic_w, y_end+(_pic_f/2)+_pic_h, screenDrawPixel);
    int ver_line_w = _pic_f/2, ver_line_h = 1.5*_pic_h;
    int ver_line_x_str = (SCREEN_WIDTH - ver_line_w) / 2, ver_line_y_str = hor_line_y_str;
    y_end = ver_line_y_str + ver_line_h - (_pic_h/2) - (_pic_f/2);
    _pic_x_str = (SCREEN_WIDTH - _pic_w - _pic_f) / 2;
    if(inference_status != 'i'){
      String highlight_pic = (inference_status == 'd') ? "assets/run_inference_detected.bmp" : "assets/run_inference_not_detected.bmp";
      screen.fillRect(ver_line_x_str, ver_line_y_str, ver_line_w, ver_line_h, highlight_c);
      screen.fillRoundRect(_pic_x_str, y_end, _pic_w+_pic_f, _pic_h+_pic_f, _pic_f/2, highlight_c);
      decoder.drawPicture(highlight_pic.c_str(), _pic_x_str+(_pic_f/2), y_end+(_pic_f/2), _pic_x_str+(_pic_f/2)+_pic_w, y_end+(_pic_f/2)+_pic_h, screenDrawPixel);
    }
    int l_sp = 25, l_x_str = (SCREEN_WIDTH - (highlight_mes.length()*text_m_s)) / 2;
    y_end += _pic_h + _pic_f + l_sp;
    screen.setTextColor(highlight_c);
    screen.setTextSize(2);
    screen.setCursor(l_x_str, y_end);
    screen.fillRect(0, y_end-5, SCREEN_WIDTH, text_m_s+10, c_green_1);
    screen.print(highlight_mes);
    y_end += text_m_s*2;
    if(inference_status == 'd' && vehicle_validation_status){
      String code_lora_success = "LoRa => Transferred!";
      l_x_str = (SCREEN_WIDTH - (code_lora_success.length()*text_m_s)) / 2;
      screen.setTextColor(c_edge_b);
      screen.setCursor(l_x_str, y_end);
      screen.print(code_lora_success);      
    }
    if(inference_status == 'd' && !vehicle_validation_status){
      String code_lora_success = "LoRa => Failed!";
      l_x_str = (SCREEN_WIDTH - (code_lora_success.length()*text_m_s)) / 2;
      screen.setTextColor(c_edge_r);
      screen.setCursor(l_x_str, y_end);
      screen.print(code_lora_success); 
    }
  }  

  else if(_type == "add_vehicle"){
    // Enable showing the modified account authentication key if the user did not validate a registered vehicle.
    String _account_auth_key = (!vehicle_validation_status) ? account_auth_key : "AUTH"; 
    if(_clear) screen.fillScreen(c_green_1);
    screen.fillRoundRect(0, h_y_s, pic_w+pic_f, pic_h+pic_f, pic_f/2, c_orange_3);
    String opt_img = "assets/menu_logo_" + String(_opt) + ".bmp";
    decoder.drawPicture(opt_img.c_str(), pic_f/2, h_y_s+pic_f/2, pic_w+pic_f/2, h_y_s+pic_h+pic_f/2, screenDrawPixel);
    screen.setTextColor(c_orange_1);
    screen.setTextSize(3);
    screen.setCursor(pic_w+pic_f+11, h_y_s+pic_f);
    screen.print("Add");
    screen.setCursor(pic_w+pic_f+11, h_y_s+pic_h+pic_f-22);
    screen.print("Vehicle");
    int y_end = h_y_s+pic_h+pic_f, init_sp = 55;
    int tri_x_sp = 5, tri_w = 20, tri_h = 30, tri_sc = 4;
    int x_str = tri_x_sp + tri_w + 10;
    y_end += init_sp;
    screen.setTextColor(c_white_1);
    screen.setTextSize(2);
    screen.fillTriangle(tri_x_sp, y_end-(tri_h/2), tri_x_sp+tri_w, y_end, tri_x_sp, y_end+(tri_h/2), c_green_3);
    screen.fillTriangle(tri_x_sp+(tri_sc/2), y_end-(tri_h/2)+tri_sc, tri_x_sp+tri_w-tri_sc, y_end, tri_x_sp+(tri_sc/2), y_end+(tri_h/2)-tri_sc, c_white_1);    
    screen.setCursor(x_str, y_end-(text_m_s/2));
    screen.print("Enter Account Key");
    int l_str = (SCREEN_WIDTH - (text_bb_s * _account_auth_key.length())) / 2;
    y_end += (tri_h/2) + (text_bb_s*1.5);
    screen.fillRect(0, y_end-5, SCREEN_WIDTH, text_bb_s+10, c_green_1);
    int _hightlight_c = (_account_auth_key.length() == 4) ? c_green_3 : c_white_1;
    screen.setTextColor(_hightlight_c);
    screen.setTextSize(4);
    screen.setCursor(l_str, y_end);
    screen.print(_account_auth_key);
    int f_text_sp = 5;
    y_end = SCREEN_HEIGHT - f_text_sp - (text_s_s*2);
    screen.fillRect(0, y_end-5, text_s_s*8, (text_s_s*2)+f_text_sp, c_green_1);
    screen.setTextSize(1);
    screen.setTextColor(c_orange_2);
    screen.setCursor(f_text_sp, y_end);
    String cur_keypad = (current_keypad_map == 1) ? "K: 123" : "K: ABC";
    screen.print(cur_keypad);
    String auth_status = (vehicle_validation_status) ? "VEHICLE: AUTHORIZED" : "VEHICLE: NO_AUTH";
    int auth_color = (vehicle_validation_status) ? c_green_3 : c_red_1;
    int f_text_right_x_str = SCREEN_WIDTH - (auth_status.length()*text_s_s) - f_text_sp;
    screen.setTextColor(auth_color);
    screen.setCursor(f_text_right_x_str, y_end);
    screen.print(auth_status);
  }

  else if(_type == "add_vehicle_img_collect"){
    int _pic_w = 80, _pic_h = 80, _pic_f = 10, _pic_y_sp = 30;
    int y_sp = 15, y_l_sp = 30;
    int y_end = y_sp;
    if(_clear) screen.fillScreen(c_green_1);
    screen.setCursor(0, y_end);
    screen.setTextColor(c_blue_1);
    screen.setTextSize(3);
    screen.print("Account: " + account_auth_key);
    y_end += text_b_s + _pic_y_sp;
    int _p_pic_x_str = (SCREEN_WIDTH - _pic_w) / 2;
    screen.fillRoundRect(_p_pic_x_str-(_pic_f/2), y_end-(_pic_f/2), _pic_w+_pic_f, _pic_h+_pic_f, _pic_f, c_blue_2);
    decoder.drawPicture("assets/camera_icon.bmp", _p_pic_x_str, y_end, _p_pic_x_str+_pic_w, y_end+_pic_h, screenDrawPixel);
    y_end += _pic_h + (_pic_f/2) + _pic_y_sp - 10;
    int x_str = 15, l_sp = 12;
    int img_status_c = (img_save_status[0] == 'w') ? c_orange_2 : ((img_save_status[0] == 'f') ? c_red_1 : c_green_3);
    String img_file_status = (img_save_status[0] == 'w') ? "Capturing sample..." : ((img_save_status[0] == 'f') ? "Error => Cannot capture image!" : "Successful => "+account_auth_key+"_img_sample_1.jpg");
    screen.fillRect(0, y_end-text_s_s, SCREEN_WIDTH, text_s_s*3, c_green_1);
    screen.setTextSize(1);
    screen.setCursor(x_str, y_end);
    screen.setTextColor(img_status_c);
    screen.print(img_file_status);
    for(int i = 1; i < IMG_SAMPLE_NUM; i++){
      y_end += (text_s_s*3) + l_sp;
      img_status_c = (img_save_status[i] == 'w') ? c_orange_2 : ((img_save_status[i] == 'f') ? c_red_1 : c_green_3);
      img_file_status = (img_save_status[i] == 'w') ? "Capturing sample..." : ((img_save_status[i] == 'f') ? "Error => Cannot capture image!" : "Successful => "+account_auth_key+"_img_sample_"+String(i+1)+".jpg");
      screen.fillRect(0, y_end-text_s_s, SCREEN_WIDTH, text_s_s*3, c_green_1);    
      screen.setCursor(x_str, y_end);
      screen.setTextColor(img_status_c);
      screen.print(img_file_status);  
    }
  }

  else if(_type == "reset_con"){  
    if(_clear) screen.fillScreen(c_green_1);
    screen.fillRoundRect(0, h_y_s, pic_w+pic_f, pic_h+pic_f, pic_f/2, c_orange_3);
    String opt_img = "assets/menu_logo_" + String(_opt) + ".bmp";
    decoder.drawPicture(opt_img.c_str(), pic_f/2, h_y_s+pic_f/2, pic_w+pic_f/2, h_y_s+pic_h+pic_f/2, screenDrawPixel);
    screen.setTextColor(c_orange_1);
    screen.setTextSize(3);
    screen.setCursor(pic_w+pic_f+11, h_y_s+pic_f);
    screen.print("Reset");
    screen.setCursor(pic_w+pic_f+11, h_y_s+pic_h+pic_f-22);
    screen.print("LoRa Con");
    int y_end = h_y_s+pic_h+pic_f, init_sp = 35, x_str = 10;
    int sq_w = 15, sq_x_sp = 10, sq_y_sp = 8, sq_r = 3, sq_x_str = SCREEN_WIDTH-sq_w-sq_x_sp;
    y_end += init_sp + sq_y_sp;
    int sq_y_str = y_end-((sq_w-text_s_s) / 2);
    screen.setTextSize(1);
    screen.setTextColor(c_blue_4);
    screen.setCursor(x_str, y_end);
    screen.print("AT+CJOINMODE");
    int status_c = (ttn_settings.con_status[0] == 'w') ? c_orange_2 : ((ttn_settings.con_status[0] == 'f') ? c_red_1 : c_green_3);
    screen.fillRoundRect(sq_x_str, sq_y_str, sq_w, sq_w, sq_r, status_c);
    y_end += text_s_s + (sq_w/2) + sq_y_sp;
    sq_y_str = y_end-((sq_w-text_s_s) / 2);
    screen.setCursor(x_str, y_end);
    screen.print("AT+CFREQBANDMASK");
    status_c = (ttn_settings.con_status[1] == 'w') ? c_orange_2 : ((ttn_settings.con_status[1] == 'f') ? c_red_1 : c_green_3);
    screen.fillRoundRect(sq_x_str, sq_y_str, sq_w, sq_w, sq_r, status_c);
    y_end += text_s_s + (sq_w/2) + sq_y_sp;
    sq_y_str = y_end-((sq_w-text_s_s) / 2);
    screen.setCursor(x_str, y_end);
    screen.print("AT+CCLASS");
    status_c = (ttn_settings.con_status[2] == 'w') ? c_orange_2 : ((ttn_settings.con_status[2] == 'f') ? c_red_1 : c_green_3);
    screen.fillRoundRect(sq_x_str, sq_y_str, sq_w, sq_w, sq_r, status_c);
    y_end += text_s_s + (sq_w/2) + sq_y_sp;
    sq_y_str = y_end-((sq_w-text_s_s) / 2);
    screen.setCursor(x_str, y_end);
    screen.print("AT+CDEVEUI");
    status_c = (ttn_settings.con_status[3] == 'w') ? c_orange_2 : ((ttn_settings.con_status[3] == 'f') ? c_red_1 : c_green_3);
    screen.fillRoundRect(sq_x_str, sq_y_str, sq_w, sq_w, sq_r, status_c);  
    y_end += text_s_s + (sq_w/2) + sq_y_sp;
    sq_y_str = y_end-((sq_w-text_s_s) / 2);
    screen.setCursor(x_str, y_end);
    screen.print("AT+CAPPEUI");
    status_c = (ttn_settings.con_status[4] == 'w') ? c_orange_2 : ((ttn_settings.con_status[4] == 'f') ? c_red_1 : c_green_3);
    screen.fillRoundRect(sq_x_str, sq_y_str, sq_w, sq_w, sq_r, status_c);     
    y_end += text_s_s + (sq_w/2) + sq_y_sp;
    sq_y_str = y_end-((sq_w-text_s_s) / 2);
    screen.setCursor(x_str, y_end);
    screen.print("AT+CAPPKEY");
    status_c = (ttn_settings.con_status[5] == 'w') ? c_orange_2 : ((ttn_settings.con_status[5] == 'f') ? c_red_1 : c_green_3);
    screen.fillRoundRect(sq_x_str, sq_y_str, sq_w, sq_w, sq_r, status_c);    
    y_end += text_s_s + (sq_w/2) + sq_y_sp;
    sq_y_str = y_end-((sq_w-text_s_s) / 2);
    screen.setCursor(x_str, y_end);
    screen.print("AT+CULDLMODE");
    status_c = (ttn_settings.con_status[6] == 'w') ? c_orange_2 : ((ttn_settings.con_status[6] == 'f') ? c_red_1 : c_green_3);
    screen.fillRoundRect(sq_x_str, sq_y_str, sq_w, sq_w, sq_r, status_c);    
    y_end += text_s_s + (sq_w/2) + sq_y_sp;
    sq_y_str = y_end-((sq_w-text_s_s) / 2);
    screen.setCursor(x_str, y_end);
    screen.print("AT+CJOIN");
    status_c = (ttn_settings.con_status[7] == 'w') ? c_orange_2 : ((ttn_settings.con_status[7] == 'f') ? c_red_1 : c_green_3);
    screen.fillRoundRect(sq_x_str, sq_y_str, sq_w, sq_w, sq_r, status_c);
  }  
}

void screenDrawPixel(int16_t x, int16_t y, uint16_t color){
  // Draw a pixel (point) on the Fermion TFT LCD display.
  screen.writePixel(x,y,color);
}

void adjustColor(int r, int g, int b){
  analogWrite(red_pin, (255-r));
  analogWrite(green_pin, (255-g));
  analogWrite(blue_pin, (255-b));
}