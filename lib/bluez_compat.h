
/* Here are the defines that make it possible to
 * implement bluetooth with the Bluez API
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _WIN32
/* you need the headers files from the Platform SDK */
#include <winsock2.h>
#include <ws2bth.h>
#define bdaddr_t    BTH_ADDR
#define sockaddr_rc _SOCKADDR_BTH
#define rc_family   addressFamily
#define rc_bdaddr   btAddr
#define rc_channel  port
#define PF_BLUETOOTH   PF_BTH
#define AF_BLUETOOTH   PF_BLUETOOTH
#define BTPROTO_RFCOMM BTHPROTO_RFCOMM
static bdaddr_t bdaddr_any = BTH_ADDR_NULL;
#define BDADDR_ANY     &bdaddr_any
#define bacpy(dst,src) memcpy((dst),(src),sizeof(BTH_ADDR))
#define bacmp(a,b)     memcmp((a),(b),sizeof(BTH_ADDR))

#else /* _WIN32 */
/* Linux/FreeBSD/NetBSD case */

#if defined(HAVE_BLUETOOTH_LINUX)
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#elif defined(HAVE_BLUETOOTH_FREEBSD)
#include <bluetooth.h>
#define sockaddr_rc sockaddr_rfcomm
#define rc_family   rfcomm_family
#define rc_bdaddr   rfcomm_bdaddr
#define rc_channel  rfcomm_channel

#elif defined(HAVE_BLUETOOTH_NETBSD)
#include <bluetooth.h>
#include <netbt/rfcomm.h>
#define sockaddr_rc sockaddr_bt
#define rc_family   bt_family
#define rc_bdaddr   bt_bdaddr
#define rc_channel  bt_channel
#define BDADDR_ANY  NG_HCI_BDADDR_ANY

#endif /* HAVE_BLUETOOTH_* */

#endif /* _WIN32 */
