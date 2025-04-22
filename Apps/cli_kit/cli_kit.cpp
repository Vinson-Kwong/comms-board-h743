#include "cli_kit.h"

TL_ErrCode CliKitInit(void)
{
    CliLog();
    CliMotor();
    return TL_E_OK;
}
