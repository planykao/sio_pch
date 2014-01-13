#ifndef _MSI_GPIO_LOOPBACK_H
#define _MSI_GPIO_LOOPBACK_H

/* Global defination */
#define GPIO_HIGH 1
#define GPIO_LOW  0

#ifdef DEBUG
#define DBG(format, args...) \
        printf("%s[%d]: "format, __func__, __LINE__, ##args)
#else
#define DBG(args...)
#endif

#define ERR(format, args...) \
        printf("%s[%d]: "format, __func__, __LINE__, ##args)

#endif
