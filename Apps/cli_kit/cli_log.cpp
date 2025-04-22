#include "cmdline.h"
#include "tl_log.h"
#include "cli_kit.h"

static const char g_logLevel[][8] = {"fatal", "error", "warn", "info", "debug", "trace"};

static struct shell_cmd_help SetLogLevelHelp = {"set log level", "set level [fatal/error/warn/info/debug/trace]", NULL};
static int SetLogLevel(int argc, char *argv[], shell_cmd_output_func_t output)
{
    if (argc < 1) {
        output("Invalid argument\r\n");
    }
    for (unsigned int i = 0; i < SIZEOF_ARRAY(g_logLevel); i++) {
        if (strlen(argv[0]) == strlen(g_logLevel[i]) && strcmp(argv[0], g_logLevel[i]) == 0) {
            TL_LogSetLevel(i);
            return 0;
        }
    }
    return -1;
}

static struct shell_cmd_help GetLogLevelHelp = {"get log level", "get level", NULL};
static int GetLogLevel(int argc, char *argv[], shell_cmd_output_func_t output)
{
    uint8_t level = TL_LogGetLevel();
    output("log level: %s\r\n", g_logLevel[level]);
    return 0;
}

static const struct shell_cmd cliLogTable[] = {{"set level", NULL, &SetLogLevelHelp, SetLogLevel},
                                               {"get level", NULL, &GetLogLevelHelp, GetLogLevel},
                                               {NULL, NULL, NULL, NULL}};
void CliLog(void)
{
    cmdline_register("log", cliLogTable);
    return;
}
