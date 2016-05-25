/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include <project.h>


#include <stdio.h>

CY_ISR(isr_bootloader) {
    UART_1_Stop();
    Bootloadable_1_Load();
}

// ADC readings when terminals are connected together
#define ADC_OFFSET_0 -20
#define ADC_OFFSET_1 -18
#define ADC_OFFSET_2 -18
// Op-amp gain factor (Uncalibrated = 11 = 1 + 10k / 1k)
// These values are times 100
#define OPAMP_GAIN_FACTOR_0 1060
#define OPAMP_GAIN_FACTOR_1 1060
#define OPAMP_GAIN_FACTOR_2 1060
// Temperature reading for cold junction
#define ROOM_TEMPERATURE (22 * 100)

char serialBuf[100];

void SendTemperature(uint8 channel, int16 adcOffset, int32 opampGainFactor) {
#define NUM_ADC_READINGS 16
    // Select the desired thermocouple input
    AMux_1_Select(channel);
    
    // Wait for op-amp to settle
    CyDelay(1);
    
    // Discard a result from ADC (probably unnecessary)
    ADC_SAR_Seq_1_IsEndConversion(ADC_SAR_Seq_1_WAIT_FOR_RESULT);
    
    // Average together a bunch of ADC readings
    // (note the hardware is already averaging 256 samples at a time)
    int32 adcReadingTotal = 0;
    uint8 i = 0;
    for (i = 0; i < NUM_ADC_READINGS; i++) {
        ADC_SAR_Seq_1_IsEndConversion(ADC_SAR_Seq_1_WAIT_FOR_RESULT);
        adcReadingTotal += ADC_SAR_Seq_1_GetResult16(0) - adcOffset;
    }
    int16 adcReading = adcReadingTotal / NUM_ADC_READINGS;
    
    // Convert to microvolts
    int32 hotJunctionVolts = ADC_SAR_Seq_1_CountsTo_uVolts(0, adcReading);
    
    // Reading was amplified by op-amp
    hotJunctionVolts = hotJunctionVolts * 100 / opampGainFactor;
    
    // Adjust for temperature at cold side of thermocouple
    int32 coldJunctionVolts = Thermocouple_1_GetVoltage(ROOM_TEMPERATURE);
    
    // Read temperature (100 * degrees C)
    int32 temperature = Thermocouple_1_GetTemperature(hotJunctionVolts + coldJunctionVolts);
    
    // Send over serial
    sprintf(
        serialBuf,
        "T%d=%ld (adc=%hd uvHot=%ld uvCold=%ld)\r\n",
        channel + 1,
        temperature,
        adcReading, hotJunctionVolts, coldJunctionVolts
    );
    UART_1_UartPutString(serialBuf);
}

uint8 relayOnCycles = 0;
uint8 relayState = 0;

void SetRelays(uint8 state) {
    relayState = state ? 1 : 0;
    // Pin is connected to a PNP transistor so LOW = relays ON
    Pin_Relays_Write(1 - relayState);
    // Built-in LED works as expected
    Pin_Builtin_LED_Write(relayState);
    // Reset relay-on cycle count
    relayOnCycles = 0;
}

void ProcessSerialCommand(char * command) {
    if (strcmp(command, "ON\n") == 0) {
        UART_1_UartPutString("R=1 (command: ON)\r\n");
        SetRelays(1);
    } else if (strcmp(command, "OFF\n") == 0) {
        UART_1_UartPutString("R=0 (command: OFF)\r\n");
        SetRelays(0);
    } else {
        uint16 len = sprintf(serialBuf, "Received unknown command (");
        uint16 i = 0;
        char c;
        while ((c = command[i++]) > 0) {
            if (c >= 32 && c <= 126) {
                serialBuf[len++] = c;
            } else {
                len += sprintf(serialBuf + len, "\\x%02x", c);
            }
        }
        len += sprintf(serialBuf + len, ")\r\n");
        serialBuf[len] = 0;
        UART_1_UartPutString(serialBuf);
    }
}

void ReadSerialInput() {
#define MAX_SERIAL_COMMAND_LENGTH 8
static char rxBuf[MAX_SERIAL_COMMAND_LENGTH + 2];
static uint8 rxLength = 0;

    while ((rxBuf[rxLength] = UART_1_UartGetChar()) > 0) {
        if (rxBuf[rxLength] > 0 && rxBuf[rxLength] != '\r') {
            // Received a character (ignore \r)
            rxLength++;
            if (rxBuf[rxLength - 1] == '\n') {
                // Received a command - ensure the buffer is null-terminated,
                // then process the command
                rxBuf[rxLength] = 0;
                ProcessSerialCommand(rxBuf);
                // Reset the buffer
                rxLength = 0;
            } else if (rxLength >= MAX_SERIAL_COMMAND_LENGTH) {
                // This line is too long; ignore it
                // (Set the first byte to zero and keep overwriting the
                // remaining bytes for as long as necessary)
                rxBuf[0] = 0;
                rxLength = 1;
            }
        }
    }
}

int main()
{
    // Enable global interrupts
    CyGlobalIntEnable;
    
    // Enable bootloader button
    // (need to clear pending interrupt first)
    isr_1_ClearPending();
    isr_1_StartEx(isr_bootloader);
    
    // Enable analog multiplexer (chooses thermocouple input)
    AMux_1_Start();
    
    // Enable op-amp (amplifies thermocouple signal)
    Opamp_1_Start();
    
    // Enable ADC (reads amplified signal)
    ADC_SAR_Seq_1_Start();
    ADC_SAR_Seq_1_StartConvert();
    
    // Enable serial transmission
    UART_1_Start();
    
    // To avoid a flash on power-on, the relay pin is configured as a
    // high impedance input initially
    // Because some guy on the internet says so:
    // http://www.cypress.com/forum/psoc-4-architecture/low-initial-drive-state-pwm-components
    Pin_Relays_Write(1); // HIGH to turn PNP transistor OFF
    Pin_Relays_SetDriveMode(Pin_Relays_DM_STRONG);
    
    for(;;) {
        // Read thermocouples and send over serial
        // Note that externally (and over serial communication),
        // 0 is T1, 1 is T2, and 2 is T3
        SendTemperature(0, ADC_OFFSET_0, OPAMP_GAIN_FACTOR_0);
        SendTemperature(1, ADC_OFFSET_1, OPAMP_GAIN_FACTOR_1);
        SendTemperature(2, ADC_OFFSET_2, OPAMP_GAIN_FACTOR_2);
        
        // Newline for easier debugging
        UART_1_UartPutString("\r\n");
        
        // Wait a bit, processing serial command input in between delays
        uint8 i;
        for (i = 0; i < 5; i++) {
            ReadSerialInput();
            CyDelay(100);
        }
        
        // Safety feature: Ensure that the relays do not stay on for more than
        // ~30 seconds without communication from the serial control program
        if (relayState) {
            if (++relayOnCycles >= 60) {
                UART_1_UartPutString("R=0 (inactivity)\r\n");
                SetRelays(0);
            }
        } else {
            relayOnCycles = 0;
        }
    }
}

/* [] END OF FILE */
