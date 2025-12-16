#ifndef PROCESS_H
#define PROCESS_H

// FreeRTOS task for processing sensor data
// Priority 4 - runs event detection and batches telemetry
void processing_task(void *pvParameters);

#endif // PROCESS_H
