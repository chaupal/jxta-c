#ifndef __RDV_MONITOR_ENTRY_H__
#define __RDV_MONITOR_ENTRY_H__

#include "jstring.h"
#include "jxta_id.h"
#include "jxta_vector.h"
#include "jxta_object_type.h"



#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif


typedef struct _jxta_rdv_monitor_entry Jxta_rdv_monitor_entry;

typedef struct _jxta_rdv_monitor_client_entry { 
	Extends(Jxta_object);
	JString *name;
	JString *pid;
	JString *addr;
} Jxta_rdv_monitor_client_entry;

JXTA_DECLARE (Jxta_rdv_monitor_entry *) jxta_rdv_monitor_entry_new(void);
JXTA_DECLARE(Jxta_status) jxta_rdv_monitor_entry_get_xml(Jxta_rdv_monitor_entry *myself, JString **xml);

JXTA_DECLARE(Jxta_id*) jxta_rdv_monitor_entry_get_src_peer_id(Jxta_rdv_monitor_entry *myself);
JXTA_DECLARE(void) jxta_rdv_monitor_entry_set_src_peer_id(Jxta_rdv_monitor_entry *myself, Jxta_id * src_peer_id);

JXTA_DECLARE(void) jxta_rdv_monitor_entry_set_clients(Jxta_rdv_monitor_entry *myself, Jxta_vector *clients);
JXTA_DECLARE(void) jxta_rdv_monitor_entry_get_clients(Jxta_rdv_monitor_entry *myself, Jxta_vector **clients);

Jxta_status jxta_rdv_monitor_entry_add_client_2(Jxta_rdv_monitor_entry *myself, JString * pid, JString* name, JString* ea);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __RDV_MONITOR_ENTRY_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
