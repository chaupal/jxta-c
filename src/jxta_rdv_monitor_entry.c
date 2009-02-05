static const char * __log_cat = "RdvMonEntry";


#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"

#include "jxta_monitor_service.h"
#include "jxta_rdv_monitor_entry.h"


enum tokentype {
	Null_,
	RdvRequestMsg_,
	client_
};


struct _jxta_rdv_monitor_entry {
	Jxta_advertisement advertisement;
	Jxta_id *src_peer_id;
	Jxta_vector *clients;
};



static void client_entry_delete(Jxta_object *me);
static Jxta_rdv_monitor_client_entry *client_entry_construct(Jxta_rdv_monitor_client_entry *myself);
static void handle_rdv_monitor_entry(void *me, const XML_Char * cd, int len);
static void handle_client_entry(void *me, const XML_Char * cd, int len);
void client_entry_destruct(Jxta_rdv_monitor_client_entry *self);
static void rdv_monitor_entry_delete(Jxta_object* me);
static Jxta_rdv_monitor_entry *rdv_monitor_entry_construct(Jxta_rdv_monitor_entry *myself);
  
 

static Jxta_rdv_monitor_client_entry *client_entry_new(void)
{
	Jxta_rdv_monitor_client_entry *self = (Jxta_rdv_monitor_client_entry*)calloc(1, sizeof(Jxta_rdv_monitor_client_entry));

	JXTA_OBJECT_INIT(self, client_entry_delete, 0);
	
	return client_entry_construct(self);
}

static void client_entry_delete(Jxta_object *entry)
{
	Jxta_rdv_monitor_client_entry * self = (Jxta_rdv_monitor_client_entry*) entry;

	client_entry_destruct(self);
	free(self);
}


Jxta_rdv_monitor_client_entry * client_entry_construct(Jxta_rdv_monitor_client_entry *self)
{
	self->thisType = "Jxta_rdv_monitor_client_entry";
	self->name = NULL;
	self->pid = NULL;
	self->addr = NULL;
	return self;
}

void client_entry_destruct(Jxta_rdv_monitor_client_entry *self)
{
	if(self->name) {
		JXTA_OBJECT_RELEASE(self->name);
	}
	if (self->pid) {
		JXTA_OBJECT_RELEASE(self->pid);
	}
	if (self->addr) {
		JXTA_OBJECT_RELEASE(self->addr);
	}
	self->thisType = NULL;
}


JXTA_DECLARE (Jxta_rdv_monitor_entry *) jxta_rdv_monitor_entry_new(void)
{
	Jxta_rdv_monitor_entry * myself = (Jxta_rdv_monitor_entry*) calloc(1, sizeof(Jxta_rdv_monitor_entry));

	if(myself == NULL) {
		return NULL;
	}

	JXTA_OBJECT_INIT(myself, rdv_monitor_entry_delete, NULL);
	jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "RdvMonEntry NEW [%pp]\n", myself);

	return rdv_monitor_entry_construct(myself);
}

static void rdv_monitor_entry_delete(Jxta_object* me)
{
	Jxta_rdv_monitor_entry *myself = (Jxta_rdv_monitor_entry*) me;

	JXTA_OBJECT_RELEASE(myself->src_peer_id);
	if(myself->clients) {
		JXTA_OBJECT_RELEASE(myself->clients);
	}

	jxta_advertisement_destruct((Jxta_advertisement*)myself);

	memset(myself, 0xdd, sizeof(Jxta_rdv_monitor_entry));

	jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "RdvMonEntry FREE [%pp]\n", myself);

	free(myself);
}

static const Kwdtab rdv_monitor_entry_tags[] = {
	{"Null", Null_, NULL, NULL, NULL},
	{"jxta:RdvMonEntry", RdvRequestMsg_, *handle_rdv_monitor_entry, NULL, NULL},
	{"Client", client_, *handle_client_entry, NULL, NULL},
	{NULL, 0, 0, NULL, NULL}
};

static Jxta_rdv_monitor_entry *rdv_monitor_entry_construct(Jxta_rdv_monitor_entry *myself)
{
	myself = (Jxta_rdv_monitor_entry*)
				jxta_advertisement_construct((Jxta_advertisement*) myself,
												"jxta:RdvMonEntry",
												rdv_monitor_entry_tags,
												(JxtaAdvertisementGetXMLFunc) jxta_rdv_monitor_entry_get_xml,
												(JxtaAdvertisementGetIDFunc) NULL,
												(JxtaAdvertisementGetIndexFunc) NULL);
	if(NULL != myself) {
		myself->src_peer_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
		myself->clients = jxta_vector_new(0);
	}

	return myself;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_monitor_entry_get_xml(Jxta_rdv_monitor_entry *myself, JString **xml)
{
	Jxta_status res = JXTA_SUCCESS;
	JString * string;
	JString * tempstr;

	if ( xml == NULL) {
		return JXTA_INVALID_ARGUMENT;
	}

	string = jstring_new_0();
	
	jstring_append_2(string, "<jxta:RdvMonEntry ");
	jstring_append_2(string, " src_id=\"");
	jxta_id_to_jstring(myself->src_peer_id, &tempstr);
	jstring_append_1(string, tempstr);
	jstring_append_2(string, "\"");
	JXTA_OBJECT_RELEASE(tempstr);
	jstring_append_2(string, ">\n ");
	
	unsigned int i;
	for(i=0; i<jxta_vector_size(myself->clients); i++) {
		Jxta_rdv_monitor_client_entry * clientEntry = NULL;
		jxta_vector_get_object_at(myself->clients, JXTA_OBJECT_PPTR(&clientEntry), i);

		jstring_append_2(string, "<Client ");
		
		JString * emptyString = jstring_new_2("");
		//add the client peer name
		if( clientEntry->name != NULL && jstring_equals(clientEntry->name, emptyString) != 0 ) {
			jstring_append_2(string, "name=\"");
			jstring_append_1(string, clientEntry->name);
			jstring_append_2(string, "\" ");
		}

		//add the client address
		if( clientEntry->addr != NULL && jstring_equals(clientEntry->addr, emptyString) != 0) {
			jstring_append_2(string, "addr=\"");
			jstring_append_1(string, clientEntry->addr);
			jstring_append_2(string, "\" ");
		}

		//add the client id
		if( clientEntry->pid != NULL && jstring_equals(clientEntry->pid, emptyString) != 0) {
			jstring_append_2(string, "pid=\"");
			jstring_append_1(string, clientEntry->pid);
			jstring_append_2(string, "\" ");
		}

		jstring_append_2(string, "/>\n");

		JXTA_OBJECT_RELEASE(clientEntry);
		JXTA_OBJECT_RELEASE(emptyString);
	}
	jstring_append_2(string, "</jxta:RdvMonEntry>\n");

	*xml = string;
	return res;
}

JXTA_DECLARE(Jxta_id*) jxta_rdv_monitor_entry_get_src_peer_id(Jxta_rdv_monitor_entry *myself)
{
	JXTA_OBJECT_CHECK_VALID(myself);

	return JXTA_OBJECT_SHARE(myself->src_peer_id);
}

JXTA_DECLARE(void) jxta_rdv_monitor_entry_set_src_peer_id(Jxta_rdv_monitor_entry *myself, Jxta_id * src_peer_id)
{
	JXTA_OBJECT_CHECK_VALID(myself);
	JXTA_OBJECT_CHECK_VALID(src_peer_id);
	if(NULL != myself->src_peer_id) {
		JXTA_OBJECT_RELEASE(myself->src_peer_id);
	}

	myself->src_peer_id = JXTA_OBJECT_SHARE(src_peer_id);
}

JXTA_DECLARE(void) jxta_rdv_monitor_entry_set_clients(Jxta_rdv_monitor_entry *myself, Jxta_vector *clients)
{
	if (NULL != myself->clients) {
		JXTA_OBJECT_RELEASE(myself->clients);
	}
	myself->clients = JXTA_OBJECT_SHARE(clients);
}

JXTA_DECLARE(void) jxta_rdv_monitor_entry_get_clients(Jxta_rdv_monitor_entry *myself, Jxta_vector **clients)
{
	if (NULL != myself->clients) {
		*clients = JXTA_OBJECT_SHARE(myself->clients);
	} else {
		*clients = NULL;
	}
}

Jxta_status jxta_rdv_monitor_entry_add_client_entry(Jxta_rdv_monitor_entry *myself, Jxta_rdv_monitor_client_entry *entry)
{
	JXTA_OBJECT_CHECK_VALID(myself);
	JXTA_OBJECT_CHECK_VALID(entry);
	Jxta_status status = JXTA_FAILED;

	if(NULL != myself->clients) {
		status = jxta_vector_add_object_last(myself->clients, (Jxta_object*)entry);
	}

	return status;
}

Jxta_status jxta_rdv_monitor_entry_add_client_2(Jxta_rdv_monitor_entry *myself, JString * pid, JString* name, JString* ea)
{
	JXTA_OBJECT_CHECK_VALID(myself);
	Jxta_status status = JXTA_SUCCESS;

	Jxta_rdv_monitor_client_entry *entry = client_entry_new(); 
	if(name != NULL)
		entry->name = JXTA_OBJECT_SHARE(name);
	
	if(pid != NULL)
		entry->pid = JXTA_OBJECT_SHARE(pid);

	if(ea != NULL)
		entry->addr = JXTA_OBJECT_SHARE(ea);

	status = jxta_rdv_monitor_entry_add_client_entry(myself, entry);

	JXTA_OBJECT_RELEASE(entry);

	return status;
}

static void handle_rdv_monitor_entry(void *me, const XML_Char * cd, int len)
{
	Jxta_rdv_monitor_entry *myself  = (Jxta_rdv_monitor_entry*) me;

	JXTA_OBJECT_CHECK_VALID(myself);

	if(0 == len) {
		const char ** atts = ((Jxta_advertisement*) myself)->atts;

		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:RdvMonEntry> : [%pp]\n", myself);

		while(atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
			} else if ( 0 == strcmp(*atts, "src_id")) {
				JString *idstr = jstring_new_2(atts[1]);
				jstring_trim(idstr);
				JXTA_OBJECT_RELEASE(myself->src_peer_id);
				myself->src_peer_id = NULL;

				if(JXTA_SUCCESS != jxta_id_from_jstring(&myself->src_peer_id, idstr)) {
					jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE
											"ID parse failure for src peer id [%pp]\n", myself);
					myself->src_peer_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
				}
				JXTA_OBJECT_RELEASE(idstr);
			} else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
			}
			atts += 2;
		}
	 } else {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:RdvMonEntry> : [%pp]\n", myself);
	 }
}

static void handle_client_entry(void *me, const XML_Char * cd, int len)
{
	Jxta_rdv_monitor_entry * myself = (Jxta_rdv_monitor_entry*) me;
	JXTA_OBJECT_CHECK_VALID(myself);
	if(0 == len) {
	
		const char **atts = ((Jxta_advertisement*) myself)->atts;
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <Client> : [%pp]\n", myself);
	
		Jxta_rdv_monitor_client_entry * newClient = client_entry_new(); 
	  
		while (atts && *atts) {
			if ( 0 == strcmp(*atts, "name")) {
				newClient->name = jstring_new_2(atts[1]);
			} else if (0 == strcmp(*atts, "pid")) {
				newClient->pid = jstring_new_2(atts[1]);
			} else if (0 == strcmp(*atts, "addr")) {
				newClient->addr = jstring_new_2(atts[1]);
			} else {
				jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
			}
			atts += 2;
		}
		jxta_rdv_monitor_entry_add_client_entry(myself, newClient);
		JXTA_OBJECT_RELEASE(newClient);
		
	} else {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <client> : [%pp]\n", myself);
	}
}
