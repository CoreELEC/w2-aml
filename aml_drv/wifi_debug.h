#ifndef _AML_DEBUG_H_
#define _AML_DEBUG_H_

#include <linux/bitops.h>
#include <linux/kernel.h>

#ifndef BIT
#define BIT(n)    (1UL << (n))
#endif //BIT

enum
{
    AML_DBG_MODULES_TX = BIT(0),        /* tx */
    AML_DBG_MODULES_RX = BIT(1),        /* rx */
};

enum
{
    AML_DBG_OFF = 0,
    AML_DBG_ON = 1,
};

extern int aml_debug;
extern unsigned long long g_dbg_modules;

#define AML_PRINT( _m,format,...) do { \
            if (g_dbg_modules & (_m)) \
            { \
                if(_m == AML_DBG_MODULES_TX) \
                printk("[TX] <%s> %d "format"",__FUNCTION__, __LINE__, ##__VA_ARGS__); \
                if(_m == AML_DBG_MODULES_RX) \
                printk("[RX] <%s> %d "format"",__FUNCTION__, __LINE__, ##__VA_ARGS__); \
            } \
        } while (0)

#define ERROR_DEBUG_OUT(format,...) do { \
                 printk("FUNCTION: %s LINE: %d:"format"",__FUNCTION__, __LINE__, ##__VA_ARGS__); \
        } while (0)

#define AML_OUTPUT(format,...) do { \
                 printk("<%s> %d:"format"",__FUNCTION__, __LINE__, ##__VA_ARGS__); \
        } while (0)


#endif /* _DRV_AH_INTERAL_H_ */
