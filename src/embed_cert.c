#include <stdio.h>
#include <stdlib.h>
#include "../inc/embed_cert.h"

void embed_cert(const char *usb_script, const char *usb_device, const char *signature_path){
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "sudo ./%s %s %s", usb_script, usb_device, signature_path);

    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Error running script. Return code: %d\n", ret);
    }else{
        printf("Script executed successfully.\n");
    }
}