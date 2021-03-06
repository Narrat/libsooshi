#ifndef SOOSHI_H_
#define SOOSHI_H_

#include <gio/gio.h>
#include <glib-object.h>

/* Library symbol visibilty magic */
#if defined _WIN32 || defined __CYGWIN__
  #define SOOSHI_DLL_IMPORT __declspec(dllimport)
  #define SOOSHI_DLL_EXPORT __declspec(dllexport)
  #define SOOSHI_DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define SOOSHI_DLL_IMPORT __attribute__ ((visibility ("default")))
    #define SOOSHI_DLL_EXPORT __attribute__ ((visibility ("default")))
    #define SOOSHI_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define SOOSHI_DLL_IMPORT
    #define SOOSHI_DLL_EXPORT
    #define SOOSHI_DLL_LOCAL
  #endif
#endif

#ifdef SOOSHI_DLL
    #ifdef SOOSHI_DLL_EXPORTS
        #define SOOSHI_API SOOSHI_DLL_EXPORT
    #else
        #define SOOSHI_API SOOSHI_DLL_IMPORT
    #endif

    #define SOOSHI_LOCAL SOOSHI_DLL_LOCAL
#else
    #define SOOSHI_API
    #define SOOSHI_LOCAL
#endif

/* Debugging */
#define SOOSHI_START_PROFILE(x) gint64 __##x##__ = g_get_real_time();
#define SOOSHI_STOP_PROFILE(x) g_info("PROFILE '%s': %.4fms", #x, (g_get_real_time() - __##x##__) / 1000.0);

/* Constants */
#define BLUEZ_NAME "org.bluez"
#define BLUEZ_ADAPTER_INTERFACE "org.bluez.Adapter1"
#define BLUEZ_DEVICE_INTERFACE "org.bluez.Device1"
#define BLUEZ_GATT_CHARACTERISTIC_INTERFACE "org.bluez.GattCharacteristic1"

#define METER_SERVICE_UUID "1BC5FFA0-0200-62AB-E411-F254E005DBD4"
#define METER_SERIAL_IN    "1BC5FFA1-0200-62AB-E411-F254E005DBD4"
#define METER_SERIAL_OUT   "1BC5FFA2-0200-62AB-E411-F254E005DBD4"

typedef guint32 crc32_t;
#define CRC32_POLYNOMIAL          0x04C11DB7
#define CRC32_INITIAL_REMAINDER   0xFFFFFFFF
#define CRC32_FINAL_XOR_VALUE     0xFFFFFFFF
#define CRC32_CHECK_VALUE         0xCBF43926
#define CRC32_WIDTH               (8 * sizeof(crc32_t))
#define CRC32_TOPBIT              (1 << (CRC32_WIDTH - 1))

/* Error handling */
typedef enum
{
  SOOSHI_ERROR_SUCCESS,
  SOOSHI_ERROR_NO_ADAPTER_FOUND,
  SOOSHI_ERROR_SCAN_FAILED,
  SOOSHI_ERROR_DBUS_CONNECTION_FAILED
} sooshi_error_t;

SOOSHI_API extern const gchar* const __SOOSHI_ERROR_STR[];
#define sooshi_error_message(x) (__SOOSHI_ERROR_STR[(x)])

/* Tree Data Types */
typedef enum
{
    NOTSET = -1,
    PLAIN,
    LINK,
    CHOOSER,
    VAL_U8,
    VAL_U16,
    VAL_U32,
    VAL_S8,
    VAL_S16,
    VAL_S32,
    VAL_STR,
    VAL_BIN,
    VAL_FLT
} SOOSHI_NODE_TYPE;

extern const gchar* const __SOOSHI_NODE_TYPE_STR[];
#define SOOSHI_NODE_TYPE_TO_STR(x) (__SOOSHI_NODE_TYPE_STR[(x)+1])

/* Mooshi Tree Node */
typedef struct _SooshiNode SooshiNode;
struct _SooshiNode
{
    gchar *name;
    guchar op_code;
    SOOSHI_NODE_TYPE type;
    GList *children;
    SooshiNode *parent;
    gboolean has_value;

    GVariant *value;

    GList *subscriber;
};

/* Sooshi State */
typedef struct _SooshiState SooshiState;
typedef struct _SooshiStateClass SooshiStateClass;

typedef void (*sooshi_callback_t)(SooshiState *state, gpointer user_data);

struct _SooshiState
{
    GObject parent_instance;

    // org.Bluez Object Manager
    GDBusObjectManager *object_manager;

    // Bluetooth Adapter
    GDBusProxy* adapter;

    // Mooshimeter Device
    GDBusProxy* mooshimeter;
    gchar *mooshimeter_dbus_path;

    // Gatt Characteristics of Mooshimeter
    GDBusProxy *serial_in;
    GDBusProxy *serial_out;

    // Signals
    gulong properties_changed_id;
    gulong scan_signal_id;

    // Heartbeat Timer
    guint heartbeat_source_id;

    // Scan Timeout Timer
    guint scan_timeout_source_id;

    gboolean scanning;
    gboolean listening;
    gboolean connected;
    gboolean initialized;

    GMainLoop *loop;

    // Message parsing & sending
    GByteArray *buffer;
    guint send_sequence;
    guint recv_sequence;

    // Config tree root
    SooshiNode *root_node;
    GPtrArray *op_code_map;

    // CRC32 Helper
    crc32_t crc_table[256];

    // This will be called once the mooshimeter is initialized
    sooshi_callback_t init_handler;
    gpointer init_handler_data;

    // This will be called once scanning times out
    sooshi_callback_t scan_timeout_handler;
    gpointer scan_timeout_data;
};

typedef gboolean (*dbus_conditional_func_t)(GDBusInterface* interface, gpointer user_data);
typedef void (*sooshi_node_subscriber_handler_t)(SooshiState *state, SooshiNode *node, gpointer user_data);

/* Subscriber Info */
typedef struct _SooshiNodeSubscriber SooshiNodeSubscriber;
struct _SooshiNodeSubscriber
{
    guint id;
    sooshi_node_subscriber_handler_t handler;
    gpointer user_data;
};

struct _SooshiStateClass
{
    GObjectClass parent_class;
};

GType sooshi_state_get_type();

#define SOOSHI_TYPE_STATE                   (sooshi_state_get_type ())
#define SOOSHI_STATE(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), SOOSHI_TYPE_STATE, SooshiState))
#define SOOSHI_IS_STATE(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SOOSHI_TYPE_STATE))
#define SOOSHI_STATE_CLASS(_class)          (G_TYPE_CHECK_CLASS_CAST ((_class), SOOSHI_TYPE_STATE, SooshiStateClass))
#define SOOSHI_IS_STATE_CLASS(_class)       (G_TYPE_CHECK_CLASS_TYPE ((_class), SOOSHI_TYPE_STATE))
#define SOOSHI_STATE_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), SOOSHI_TYPE_STATE, SooshiStateClass))

/*****************/
/* API functions */
/*****************/
SOOSHI_API SooshiState *sooshi_state_new(sooshi_error_t *error);
SOOSHI_API void sooshi_state_delete(SooshiState *state);
SOOSHI_API sooshi_error_t sooshi_setup(SooshiState *state, sooshi_callback_t init_handler, gpointer init_data,
    sooshi_callback_t scan_timeout_handler, gpointer scan_timeout_data);
SOOSHI_API void sooshi_run(SooshiState *state);
SOOSHI_API void sooshi_stop(SooshiState *state);

// Debugging
SOOSHI_API void sooshi_debug_dump_tree(SooshiNode *node, gint indent);

// Node methods
SOOSHI_API SooshiNode *sooshi_node_find(SooshiState *state, gchar *path, SooshiNode *start);
SOOSHI_API void sooshi_node_set_value(SooshiState *state, SooshiNode *node, GVariant *value, gboolean send_update);
SOOSHI_API void sooshi_node_request_value(SooshiState *state, SooshiNode *node);
SOOSHI_API void sooshi_node_choose(SooshiState *state, SooshiNode *node);
SOOSHI_API void sooshi_node_choose_by_index(SooshiState *state, SooshiNode *node, guchar index);
SOOSHI_API guint sooshi_node_subscribe(SooshiState *state, SooshiNode *node, sooshi_node_subscriber_handler_t func, gpointer user_data);
SOOSHI_API void sooshi_node_notify_subscribers(SooshiState *state, SooshiNode *node);

/*******************/
/* Local functions */
/*******************/
SOOSHI_LOCAL GDBusProxy *sooshi_dbus_find_interface_proxy_if(SooshiState *state, const gchar* interface_name, dbus_conditional_func_t cond_func, gpointer user_data);
SOOSHI_LOCAL void sooshi_on_mooshi_initialized(SooshiState *state);
SOOSHI_LOCAL void sooshi_parse_response(SooshiState *state);
SOOSHI_LOCAL void sooshi_enable_notify(SooshiState *state);
SOOSHI_LOCAL void sooshi_send_bytes(SooshiState *state, guchar *buffer, gsize len, gboolean block);
SOOSHI_LOCAL void sooshi_request_all_node_values(SooshiState *state, SooshiNode *start);

// Debugging
SOOSHI_LOCAL gchar* sooshi_node_value_as_string(SooshiNode *node);

// Node methods
SOOSHI_LOCAL void sooshi_node_send_value(SooshiState *state, SooshiNode *node);
void sooshi_node_free_all(SooshiState *state, SooshiNode *start_node);

// Transfer helper functions
SOOSHI_LOCAL GByteArray *sooshi_node_bytes_to_value(SooshiNode *node, GByteArray *buffer, GVariant **result);
SOOSHI_LOCAL gint sooshi_node_value_to_bytes(SooshiNode *node, guchar *buffer);

// CRC-32 Stuff
SOOSHI_LOCAL void sooshi_crc32_init(SooshiState *state);
SOOSHI_LOCAL crc32_t sooshi_crc32_calculate(SooshiState *state, guchar const message[], gint nBytes);

#endif // SOOSHI_H_
