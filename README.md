# Attendance IoT Project

This project uses PlatformIO for building and managing the embedded application.

## Prerequisites

-   [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/installation/index.html) installed.
-   Python 3.x installed (for running tests).

## Setup

1.  **Configure BLE Settings**:
    *   **Device Name**:
        *   The BLE device name can be configured in `include/ble_config.h`.
        *   Edit the `BLE_DEVICE_NAME` macro if you want to change the default name ("ESP32-Signer").
    *   **Service and Characteristic UUIDs**:
        *   If you need to use specific UUIDs for your BLE service and characteristics, you can configure them in `include/uuid.h`.
        *   Modify `SERVICE_UUID`, `MESSAGE_CHARACTERISTIC_UUID`, and `SIG_CHARACTERISTIC_UUID` as needed. Example UUIDs are provided.

2.  **Configure RSA Key**:
    *   Copy the example key file:
        ```bash
        cp include/key.h.example include/key.h
        ```
    *   Edit `include/key.h` and replace the placeholder with your actual RSA key.

3.  **Build the project**:
    *   To compile the project, run:
        ```bash
        platformio run
        ```
    *   This project has pre-configured environments for ESP32 and ESP32-S3. You can specify the environment using the `-e` flag, for example:
        ```bash
        platformio run -e esp32
        ```
        or
        ```bash
        platformio run -e esp32-s3
        ```

4.  **Upload to device**:
    *   To compile and upload the firmware to your target device, run:
        ```bash
        platformio run --target upload
        ```
    *   Similarly, you can specify the environment for uploading:
        ```bash
        platformio run --target upload -e esp32
        ```

## Running

After successfully setting up and uploading the firmware to your device as described above, the application will run on the device. Monitor serial output if needed using:
```bash
platformio device monitor
```

## Testing

The tests are written in Python and are located in the `test/` directory.

1.  **Navigate to the test directory**:
    ```bash
    cd test
    ```

2.  **Install Python dependencies**:
    *   Make sure you are in the `test` directory.
    *   Install the required Python libraries:
        ```bash
        pip install -r requirements.txt
        ```
    *(Note: It's recommended to use a virtual environment for Python projects.)*

3.  **Place Public Key**:
    *   Before running the test script, ensure you have the necessary public key file placed in the `test/` directory (which should be your current working directory if you followed step 1).

4.  **Run the tests**:
    *   Execute the test script:
        ```bash
        python test.py
        ```

