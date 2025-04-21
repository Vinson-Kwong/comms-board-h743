#include <stdlib.h>
#include "cmdline.h"
#include "cybergear_wrapper.h"
#include "cli_kit.h"


static struct shell_cmd_help setMech2ZeroHelp = {"set mech2zero mode", "set mech2zero [id]", NULL};
static int SetMech2Zero(int argc, char *argv[], shell_cmd_output_func_t output)
{
    if (argc < 1) {
        output("Invalid argument\r\n");
        return 0;
    }
    int id = atoi(argv[0]);
    if (0 == id) {
        id = 0x7F; // 0x7F: default id for all motors
    }    
    int ret = cybergear_driver_set_mech_position_to_zero(id);
    output("id:%d; ret: %d\r\n", id, ret);
    return 0;
}

static const struct shell_cmd cliMotorTable[] = {
                                                {"set mech2zero", NULL, &setMech2ZeroHelp, SetMech2Zero},
                                                {NULL, NULL, NULL, NULL}};
void CliMotor(void)
{
    cmdline_register("motor", cliMotorTable);
    return;
}
