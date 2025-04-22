#include <stdlib.h>
#include "cmdline.h"
#include "cybergear.h"
#include "cli_kit.h"

#if 0
static struct shell_cmd_help getRunModeHelp = {"get run mode", "get run mode [id]", NULL};
static int GetRunMode(int argc, char *argv[], shell_cmd_output_func_t output)
{
    if (argc < 1) {
        output("Invalid argument\r\n");
        return 0;
    }
    int id = atoi(argv[0]);
    if (0 == id) {
        id = MOTOR_CAN_ID;
    }   
    int mode = CybergearGetRunMode(id);
    output("id:%d; mode: %d\r\n", id, mode);
    return 0;
}

static struct shell_cmd_help setRunModeHelp = {"set run mode", "set run mode [id] [mode](0:motion; 1:position; 2:speed; 3:current)", NULL};
static int SetRunMode(int argc, char *argv[], shell_cmd_output_func_t output)
{
    if (argc < 2) {
        output("Invalid argument\r\n");
        return 0;
    }
    int id = atoi(argv[0]);
    if (0 == id) {
        id = MOTOR_CAN_ID;
    }    
    int mode = atoi(argv[1]);
    int ret = CybergearSetRunMode(id, mode);
    output("id:%d; mode:%d; ret:%d\r\n", id, mode, ret);
    return 0;
}

static struct shell_cmd_help setRunEnaHelp = {"set run ena", "set run ena [id] [ena](0:disable; 1:enable) ", NULL};
static int SetRunEna(int argc, char *argv[], shell_cmd_output_func_t output)
{
    if (argc < 2) {
        output("Invalid argument\r\n");
        return 0;
    }
    int id = atoi(argv[0]);
    int ena = atoi(argv[1]);
    int ret = 0;
    if (0 == id) {
        id = MOTOR_CAN_ID;
    }
    if (ena) {
        ret = CybergearMotorStart(MOTOR_CAN_ID);
    }
    else {
        ret = CybergearMotorStop(MOTOR_CAN_ID, 0);    
    }    
    output("id:%d; ena:%d; ret:%d\r\n", id, ena, ret);
    return 0;
}

static struct shell_cmd_help clrErrorHelp = {"clr error", "clr error [id]", NULL};
static int ClrError(int argc, char *argv[], shell_cmd_output_func_t output)
{
    if (argc < 1) {
        output("Invalid argument\r\n");
        return 0;
    }
    int id = atoi(argv[0]);
    if (0 == id) {
        id = MOTOR_CAN_ID;
    }    
    int ret = CybergearMotorStop(id, 1);
    output("id:%d; ret: %d\r\n", id, ret);
    return 0;
}

static struct shell_cmd_help getProbeHelp = {"get probe", "get probe [id]", NULL};
static int GetProbe(int argc, char *argv[], shell_cmd_output_func_t output)
{
    if (argc < 1) {
        output("Invalid argument\r\n");
        return 0;
    }
    int id = atoi(argv[0]);
    if (0 == id) {
        id = MOTOR_CAN_ID;
    }    
    int ret = CybergearProbe(id);
    output("id:%d; ret: %d\r\n", id, ret);
    return 0;
}

static struct shell_cmd_help getStateHelp = {"get state", "get state [id]", NULL};
static int GetState(int argc, char *argv[], shell_cmd_output_func_t output)
{
    if (argc < 1) {
        output("Invalid argument\r\n");
        return 0;
    }
    int id = atoi(argv[0]);
    if (0 == id) {
        id = MOTOR_CAN_ID;
    }    
    int ret = CybergearMotorRequest(id);
    output("id:%d; ret: %d\r\n", id, ret);
    return 0;
}

static struct shell_cmd_help setHomeModeHelp = {"set home mode", "set home mode [id]", NULL};
static int SetHomeMode(int argc, char *argv[], shell_cmd_output_func_t output)
{
    if (argc < 1) {
        output("Invalid argument\r\n");
        return 0;
    }
    int id = atoi(argv[0]);
    if (0 == id) {
        id = MOTOR_CAN_ID;
    }    
    int ret = CybergearSetPosZero(id);
    output("id:%d; ret: %d\r\n", id, ret);
    return 0;
}
#endif

static const struct shell_cmd cliMotorTable[] = {
    // {"get run mode", NULL, &getRunModeHelp, GetRunMode},    {"set run mode", NULL, &setRunModeHelp, SetRunMode},
    // {"set run ena", NULL, &setRunEnaHelp, SetRunEna},       {"clr error", NULL, &clrErrorHelp, ClrError},
    // {"get probe", NULL, &getProbeHelp, GetProbe},           {"get state", NULL, &getStateHelp, GetState},
    // {"set home mode", NULL, &setHomeModeHelp, SetHomeMode}, 
    {NULL, NULL, NULL, NULL}};
void CliMotor(void)
{
    cmdline_register("motor", cliMotorTable);
    // CybergearProbe(0x7f);
    return;
}
