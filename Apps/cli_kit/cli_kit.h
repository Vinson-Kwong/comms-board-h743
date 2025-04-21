#ifndef __CLI_KIT_H__
#define __CLI_KIT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "tl_err_code.h"

TL_ErrCode CliKitInit(void);

void CliLog(void);
void CliMotor(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __AGW_CLI_H__ */