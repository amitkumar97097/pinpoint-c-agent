/*******************************************************************************
 * Copyright 2019 NAVER Corp
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations under
 * the License.
 ******************************************************************************/
#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#if defined(__GNUC__) || defined(__clang__)
#define DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED __declspec(deprecated)
#else
#define DEPRECATED
#endif

#define MAX_VEC 512
#define LOG_SIZE 4096
#define IN_MSG_BUF_SIZE 4096
#define NAMING_SIZE 128
static const int RECONNECT_TIME_SEC = 5;
typedef enum { RESPONSE_AGENT_INFO = 0, REQ_UPDATE_SPAN = 1 } MSG_TYPE;

typedef enum {
  E_LOGGING = 0x1,
  E_DISABLE_GIL = 0x2, // disable gil checking in python
  E_UTEST = 0x4
} AGENT_FLAG;

#pragma pack(1)
typedef struct {
  uint32_t type;
  uint32_t length;
} Header;
#pragma pack()

typedef struct collector_agent_s {
  uint64_t start_time;
  char* appid;
  char* appname;
} CollectorAgentInfo;

///
typedef struct trace_store_layer {
  void (*set_cur_trace_cb)(void*);
  void* (*get_cur_trace_cb)(void);
} TraceStoreLayer;

#define LOG_SIZE 4096
#define MAX_ADDRESS_SIZE 256
typedef void (*VOID_FUNC)(void);
typedef struct pp_agent_s {
  char co_host[MAX_ADDRESS_SIZE]; // tcp:ip:port should support dns
  long timeout_ms;                // always be 0
  long trace_limit;               // change to long as python need long
  int agent_type;
  uint8_t inter_flag;
  VOID_FUNC get_read_lock;
  VOID_FUNC get_write_lock;
  VOID_FUNC release_lock;
} PPAgentT;

typedef enum {
  E_OFFLINE = 0x1,
  E_TRACE_PASS = 0x2,
  E_TRACE_BLOCK = 0x4,
  E_READY = 0x8
} E_AGENT_STATUS;

/**
 * @brief at present only root checking
 */

typedef int NodeID;
typedef NodeID E_NODE_LOC;
static const NodeID E_INVALID_NODE = -1;
static const NodeID E_ROOT_NODE = 0;
static const E_NODE_LOC E_LOC_CURRENT = 0x0;
static const E_NODE_LOC E_LOC_ROOT = 0x1;

#define PINPOINT_C_AGENT_API_VERSION "0.4.25"

/**
 * @brief change logs
 * ## v0.4.13
 * pinpoint_get_context_key API changed
 * ## v0.4.10
 * - new API: pinpoint_start_traceV1,pinpoint_add_exception
 * ## v0.4.9
 * - not open
 * ## v0.4.8
 * - add tls
 */
#ifdef __cplusplus
extern "C" {
#endif

extern PPAgentT global_agent_info;

/**
 * @brief [tls]
 *  pinpoint_get_per_thread_id and pinpoint_update_per_thread_id are paired API
 *  pinpoint_get_per_thread_id get the current trace-id of current thread
 *  pinpoint_update_per_thread_id update(stores) the current trace-id for pinpoint_get_per_thread_id
 * @return NodeID
 */
NodeID pinpoint_get_per_thread_id(void);
void pinpoint_update_per_thread_id(NodeID id);

/**
 *  @brief [tls] start a trace (span) from parentId. if current span is empty, create a span or else
 * create a spanevent
 *
 * @param parentId
 * @return -1: failed new traceId related to parentId (caller [parentId] -> callee [NodeID] )
 */
NodeID pinpoint_start_trace(NodeID parentId);

/**
 * @brief [tls] V1 support trace with optional config
 *      Not support on root traceNode
 * @param parentId
 * @param opt  param ... count; if opt == nullptr, the same as pinpoint_start_trace; Note: must end
 * with nullptr
 * @param ...  only support const char*;
 * eg:
 *      "TraceMinTimeMs:23" // the minimum time of a trace should be 23ms
 *      "TraceOnlyException" // only trace exception( only report when call pinpoint_add_exception)
 * @return NodeID
 */
NodeID pinpoint_start_traceV1(NodeID parentId, const char* opt, ...);

void pinpoint_add_exception(NodeID, const char* exp);

/**[tls]
 * the same as pinpoint_start_trace. BUT, end a span or a spanevent
 * Note: pinpoint_end_trace is thread safe, but you should avoid to call it in the multi-thread, it
 * may send duplicate trace span
 * @return NodeID is parent node id
 */
NodeID pinpoint_end_trace(NodeID);

/**
 * @brief wake a finished trace: this trace get a new startime, so it could be profile again. the
 * end_trace will update the ["e"] by cumulative time (NOTE: root trace not supported)
 *
 * @param traceId
 * @return int 0 if success, -1 if failed
 */
int pinpoint_wake_trace(NodeID traceId);

/**
 *  check id->traceNode is root
 * @param
 * @return 1: is root; 0: not root node;-1: A wrong id
 */
int pinpoint_trace_is_root(NodeID);

/**
 *  force end current trace, only called when callstack leaked
 * @return int 0 : means oK
 *             -1: exception found, check the log
 */
int pinpoint_force_end_trace(NodeID, int32_t timeout);

/**
 * [tls] pinpoint_add_clues, append a value into span[key]
 * @param key must be a string
 * @param value key must be a string
 */
void pinpoint_add_clues(NodeID _id, const char* key, const char* value, E_NODE_LOC flag);
/**
 * [tls] pinpoint_add_clues, add  a key-value into span. span[key]=value
 * @param key must be a string
 * @param value key must be a string
 */
void pinpoint_add_clue(NodeID _id, const char* key, const char* value, E_NODE_LOC flag);
/**
 * [tls] add a key value into current trace. IF the trace is end, all data(key-value) will be free
 * @param key
 * @param value
 */
void pinpoint_set_context_key(NodeID _id, const char* key, const char* value);
/**
 * [tls] get the corresponding value with key(in current trace)
 * @param size copy into pbuf, not include '\0'
 * @return value size
 */
int pinpoint_get_context_key(NodeID _id, const char* key, char* pbuf, int buf_size);

void pinpoint_set_context_long(NodeID _id, const char* key, long);

/**
 * @brief the value is a long type
 *
 * @param _id  node id
 * @param key  string
 * @return int 1: failed
 *              0: success
 */
int pinpoint_get_context_long(NodeID _id, const char* key, long*);
/**
 * [tls] if tracelimit enable, check current trace state,
 * @param timestamp
 * @return 0, sampled or else, not sampled
 */
int check_tracelimit(int64_t);

/**
* @brief [tls] setting current trace status
          typedef enum {
               E_OFFLINE = 0x1,
               E_TRACE_PASS =0x2,
               E_TRACE_BLOCK =0x4,
               E_READY = 0x8
          }E_AGENT_STATUS;
* @param _id
* @param status
* @return int last status
*/
uint64_t mark_current_trace_status(NodeID _id, int status);

/**
 * [tls] get an unique auto-increment id
 * NOTE: implement by shared memory, only valid in current host.
 * @return
 */
int64_t generate_unique_id(void);

/**
 * [tls] get the start time of collector-agent.Use to generate transactionID
 * @return
 */
uint64_t pinpoint_start_time(void);

/**
 * mark current span with error
 * @param msg
 * @param error_filename
 * @param error_lineno
 */
void catch_error(NodeID _id, const char* msg, const char* error_filename, uint32_t error_lineno);
typedef void (*log_msg_cb)(char*);
void register_error_cb(log_msg_cb error_cb);

/**
 * @brief for test-case: not send span to collector-agent, pass to handler
 *  not tls
 * @param handler
 */
void register_span_handler(void (*handler)(const char*));

void pp_trace(const char* format, ...);
/**
 * NOTE: only for testcase
 */
void reset_unique_id(void);
const char* pinpoint_agent_version();
void show_status(void);
#ifdef __cplusplus
}
#endif
#endif /* COMMON_H_ */
