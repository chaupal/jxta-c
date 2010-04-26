AM_CPPFLAGS = -I$(top_srcdir)/src/jpr -I$(top_srcdir)/src @APR_CPPFLAGS@ @XML2_CPPFLAGS@ @SQLITE_CPPFLAGS@

#LIBRARY_LDFLAGS = -static

bin_PROGRAMS = jstring_test	    \
	       msg_test		    \
	       lease_msg_test       \
	       dq_adv_test	    \
	       dr_adv_test	    \
	       srdi_test	    \
	       pa_adv_test	    \
	       mia_adv_test	    \
	       pga_adv_test	    \
	       rr_adv_test	    \
	       rq_adv_test	    \
	       hta_adv_test	    \
	       svc_adv_test	    \
	       tta_adv_test	    \
	       relay_adv_test	    \
	       apa_adv_test	    \
	       route_adv_test	    \
	       rdv_adv_test	    \
	       any_adv_test	    \
	       jxta_id_test	    \
	       jxtaobject_test	    \
	       excep_test	    \
	       jxta_bytevector_test \
	       jxta_vector_test     \
	       jxta_hash_test	    \
	       jxta_objecthash_test \
	       dummypg_test	    \
	       http_client_test     \
	       jxta_xml_util_test   \
	       jxta_rdvclient_test  \
	       jxta_rdvclient_listener_test  \
	       jxta_rdvserver_test  \
	       jxta_resolver_test   \
	       jxta_discovery_test  \
               jxta_discovery_publication_test \
	       jxta_pg_test	    \
	       jxta_join_test	    \
	       endpoint_test	    \
	       endpoint_stress_test \
	       xmltest		    \
	       cm_test		    \
	       unit_test_runner	    \
	       jxta_bidipipe_test   \
	       jxta_bench_comm	    \
	       jxta_bench_pipe_resolution \
	       jxta_log_unit_test   \
	       jxta_server_tunnel   \
	       jxta_client_tunnel   \
	       pg_start_stop_test   \
	       npg_test             \
	       rdv_lease_options_test \
               endpoint_msg_test      \
	       peerview_ping_msg_test \
	       peerview_pong_msg_test \
	       peerview_address_request_msg_test \
	       peerview_address_assign_msg_test \
	       traffic_shaping_test \
               monitor_msg_test \
               rdv_diffusion_msg_test    

LDADD = \
	$(top_builddir)/src/libjxta.la \
	@APR_LIBS@		    \
        @XML2_LIBS@		    \
        @SQLITE_LIBS@

jxta_log_unit_test_SOURCES = jxta_log_unit_test.c

jxta_bidipipe_test_SOURCES = jxta_bidipipe_test.c
jxta_bench_comm_SOURCES = jxta_bench_comm.c
jxta_bench_pipe_resolution_SOURCES = jxta_bench_pipe_resolution.c
jxta_server_tunnel_SOURCES = jxta_server_tunnel.c
jxta_client_tunnel_SOURCES = jxta_client_tunnel.c

jstring_test_SOURCES = jstring_test.c unittest_jxta_func.c unittest_jxta_func.h
jstring_test.o:  jstring_test.c
	$(COMPILE) -DSTANDALONE -o jstring_test.o -c $(srcdir)/jstring_test.c

jxta_xml_util_test_SOURCES   = jxta_xml_util_test.c unittest_jxta_func.c
jxta_xml_util_test.o:  jxta_xml_util_test.c
	$(COMPILE) -DSTANDALONE -o jxta_xml_util_test.o -c $(srcdir)/jxta_xml_util_test.c

jxta_id_test_SOURCES	     = jxta_id_test.c unittest_jxta_func.c
jxta_id_test.o:  jxta_id_test.c
	$(COMPILE) -DSTANDALONE -o jxta_id_test.o -c $(srcdir)/jxta_id_test.c

dq_adv_test_SOURCES	     = dq_adv_test.c unittest_jxta_func.c
dq_adv_test.o:	dq_adv_test.c
	$(COMPILE) -DSTANDALONE -o dq_adv_test.o -c $(srcdir)/dq_adv_test.c

dr_adv_test_SOURCES	     = dr_adv_test.c
dr_adv_test.o:	dr_adv_test.c
	$(COMPILE) -DSTANDALONE -o dr_adv_test.o -c $(srcdir)/dr_adv_test.c

srdi_test_SOURCES	     = srdi_test.c
srdi_test.0:	srdi_test.c
	$(COMPILE) -DSTANDALONE -o srdi_test.0 -c $(srcdir)/srdi_test.c

rdv_diffusion_msg_test_SOURCES	     = rdv_diffusion_msg_test.c unittest_jxta_func.c
rdv_diffusion_msg_test.o:	rdv_diffusion_msg_test.c
	$(COMPILE) -DSTANDALONE -o rdv_diffusion_msg_test.o -c $(srcdir)/rdv_diffusion_msg_test.c

pa_adv_test_SOURCES	     = pa_adv_test.c unittest_jxta_func.c
pa_adv_test.o:	pa_adv_test.c
	$(COMPILE) -DSTANDALONE -o pa_adv_test.o -c $(srcdir)/pa_adv_test.c

mia_adv_test_SOURCES	     = mia_adv_test.c
mia_adv_test.o:  mia_adv_test.c
	$(COMPILE) -DSTANDALONE -o mia_adv_test.o -c $(srcdir)/mia_adv_test.c

pga_adv_test_SOURCES	     = pga_adv_test.c unittest_jxta_func.c
pga_adv_test.o:  pga_adv_test.c
	$(COMPILE) -DSTANDALONE -o pga_adv_test.o -c $(srcdir)/pga_adv_test.c

lease_msg_test_SOURCES	     = lease_msg_test.c unittest_jxta_func.c
lease_msg_test.o:  lease_msg_test.c
	$(COMPILE) -DSTANDALONE -o lease_msg_test.o -c $(srcdir)/lease_msg_test.c

rr_adv_test_SOURCES	     = rr_adv_test.c unittest_jxta_func.c
rr_adv_test.o:	rr_adv_test.c
	$(COMPILE) -DSTANDALONE -o rr_adv_test.o -c $(srcdir)/rr_adv_test.c

rq_adv_test_SOURCES	     = rq_adv_test.c unittest_jxta_func.c
rq_adv_test.o:	rq_adv_test.c
	$(COMPILE) -DSTANDALONE -o rq_adv_test.o -c $(srcdir)/rq_adv_test.c

hta_adv_test_SOURCES	     = hta_adv_test.c
hta_adv_test.o:  hta_adv_test.c
	$(COMPILE) -DSTANDALONE -o hta_adv_test.o -c $(srcdir)/hta_adv_test.c

svc_adv_test_SOURCES	     = svc_adv_test.c
svc_adv_test.o:  svc_adv_test.c
	$(COMPILE) -DSTANDALONE -o svc_adv_test.o -c $(srcdir)/svc_adv_test.c

tta_adv_test_SOURCES	     = tta_adv_test.c
tta_adv_test.o:  tta_adv_test.c
	$(COMPILE) -DSTANDALONE -o tta_adv_test.o -c $(srcdir)/tta_adv_test.c

relay_adv_test_SOURCES	       = relay_adv_test.c  unittest_jxta_func.c
relay_adv_test.o:  relay_adv_test.c
	$(COMPILE) -DSTANDALONE -o relay_adv_test.o -c $(srcdir)/relay_adv_test.c

apa_adv_test_SOURCES	     = apa_adv_test.c unittest_jxta_func.c
apa_adv_test.o:  apa_adv_test.c
	$(COMPILE) -DSTANDALONE -o apa_adv_test.o -c $(srcdir)/apa_adv_test.c

route_adv_test_SOURCES	       = route_adv_test.c unittest_jxta_func.c
route_adv_test.o:  route_adv_test.c
	$(COMPILE) -DSTANDALONE -o route_adv_test.o -c $(srcdir)/route_adv_test.c

rdv_adv_test_SOURCES	     = rdv_adv_test.c
rdv_adv_test.o:  rdv_adv_test.c
	$(COMPILE) -DSTANDALONE -o rdv_adv_test.o -c $(srcdir)/rdv_adv_test.c

any_adv_test_SOURCES	     = any_adv_test.c
any_adv_test.o:  any_adv_test.c
	$(COMPILE) -DSTANDALONE -o any_adv_test.o -c $(srcdir)/any_adv_test.c

xmltest_SOURCES		     = xmltest.c
xmltest.o:  xmltest.c
	$(COMPILE) -DSTANDALONE -o xmltest.o -c $(srcdir)/xmltest.c

msg_test_SOURCES	     = msg_test.c unittest_jxta_func.c
msg_test.o:  msg_test.c
	$(COMPILE) -DSTANDALONE -o msg_test.o -c $(srcdir)/msg_test.c

jxtaobject_test_SOURCES      = jxtaobject_test.c
jxtaobject_test.o:  jxtaobject_test.c
	$(COMPILE) -DSTANDALONE -o jxtaobject_test.o -c $(srcdir)/jxtaobject_test.c

excep_test_SOURCES	     = excep_test.c unittest_jxta_func.c
excep_test.o:  excep_test.c
	$(COMPILE) -DSTANDALONE -o excep_test.o -c $(srcdir)/excep_test.c

jxta_bytevector_test_SOURCES = jxta_bytevector_test.c unittest_jxta_func.c
jxta_bytevector_test.o:  jxta_bytevector_test.c
	$(COMPILE) -DSTANDALONE -o jxta_bytevector_test.o -c $(srcdir)/jxta_bytevector_test.c

jxta_vector_test_SOURCES     = jxta_vector_test.c  unittest_jxta_func.c
jxta_vector_test.o:  jxta_vector_test.c
	$(COMPILE) -DSTANDALONE -o jxta_vector_test.o -c $(srcdir)/jxta_vector_test.c

jxta_hash_test_SOURCES	     = jxta_hash_test.c
jxta_hash_test.o:  jxta_hash_test.c
	$(COMPILE) -DSTANDALONE -o jxta_hash_test.o -c $(srcdir)/jxta_hash_test.c

jxta_objecthash_test_SOURCES = jxta_objecthash_test.c
jxta_objecthash_test.o:  jxta_objecthash_test.c
	$(COMPILE) -DSTANDALONE -o jxta_objecthash_test.o -c $(srcdir)/jxta_objecthash_test.c

dummypg_test_SOURCES	     = dummypg_test.c dummypg_test_private.h unittest_jxta_func.c
dummypg_test.o:  dummypg_test.c 
	$(COMPILE) -DSTANDALONE -o dummypg_test.o -c $(srcdir)/dummypg_test.c

http_client_test_SOURCES     = http_client_test.c
http_client_test.o:  http_client_test.c
	$(COMPILE) -DSTANDALONE -o http_client_test.o -c $(srcdir)/http_client_test.c

jxta_rdvclient_test_SOURCES  = jxta_rdvclient_test.c
jxta_rdvclient_test.o:	jxta_rdvclient_test.c
	$(COMPILE) -DSTANDALONE -o jxta_rdvclient_test.o -c $(srcdir)/jxta_rdvclient_test.c

jxta_rdvclient_listener_test_SOURCES  = jxta_rdvclient_listener_test.c
jxta_rdvclient_listener_test.o:  jxta_rdvclient_listener_test.c
	$(COMPILE) -DSTANDALONE -o jxta_rdvclient_listener_test.o -c $(srcdir)/jxta_rdvclient_listener_test.c

jxta_rdvserver_test_SOURCES  = jxta_rdvserver_test.c
jxta_rdvserver_test.o:	jxta_rdvserver_test.c
	$(COMPILE) -DSTANDALONE -o jxta_rdvserver_test.o -c $(srcdir)/jxta_rdvserver_test.c

jxta_resolver_test_SOURCES   = jxta_resolver_test.c
jxta_resolver_test.o:  jxta_resolver_test.c
	$(COMPILE) -DSTANDALONE -o jxta_resolver_test.o -c $(srcdir)/jxta_resolver_test.c

jxta_discovery_publication_test_SOURCES   = jxta_discovery_publication_test.c
jxta_discovery_publication_test.o:  jxta_discovery_publication_test.c
	$(COMPILE) -DSTANDALONE -o jxta_discovery_publication_test.o -c $(srcdir)/jxta_discovery_publication_test.c

endpoint_test_SOURCES	     = endpoint_test.c
endpoint_test.o:  endpoint_test.c
	$(COMPILE) -DSTANDALONE -o endpoint_test.o -c $(srcdir)/endpoint_test.c

endpoint_stress_test_SOURCES = endpoint_stress_test.c
endpoint_stress_test.o:  endpoint_stress_test.c
	$(COMPILE) -DSTANDALONE -o endpoint_stress_test.o -c $(srcdir)/endpoint_stress_test.c

jxta_pg_test_SOURCES	     = jxta_pg_test.c
jxta_pg_test.o:  jxta_pg_test.c
	$(COMPILE) -DSTANDALONE -o jxta_pg_test.o -c $(srcdir)/jxta_pg_test.c

jxta_join_test_SOURCES	     = jxta_join_test.c
jxta_join_test.o:  jxta_join_test.c
	$(COMPILE) -DSTANDALONE -o jxta_join_test.o -c $(srcdir)/jxta_join_test.c

rdv_lease_options_test_SOURCES	     = rdv_lease_options_test.c unittest_jxta_func.c
rdv_lease_options_test.o:  rdv_lease_options_test.c 
	$(COMPILE) -DSTANDALONE -o rdv_lease_options_test.o -c $(srcdir)/rdv_lease_options_test.c

endpoint_msg_test_SOURCES	     = endpoint_msg_test.c unittest_jxta_func.c
endpoint_msg_test.o:  endpoint_msg_test.c 
	$(COMPILE) -DSTANDALONE -o endpoint_msg_test.o -c $(srcdir)/endpoint_msg_test.c

peerview_ping_msg_test_SOURCES	     = peerview_ping_msg_test.c unittest_jxta_func.c
peerview_ping_msg_test.o:  peerview_ping_msg_test.c 
	$(COMPILE) -DSTANDALONE -o peerview_ping_msg_test.o -c $(srcdir)/peerview_ping_msg_test.c

peerview_pong_msg_test_SOURCES	     = peerview_pong_msg_test.c unittest_jxta_func.c
peerview_pong_msg_test.o:  peerview_pong_msg_test.c 
	$(COMPILE) -DSTANDALONE -o peerview_pong_msg_test.o -c $(srcdir)/peerview_pong_msg_test.c

peerview_address_request_msg_test_SOURCES	     = peerview_address_request_msg_test.c unittest_jxta_func.c
peerview_address_request_msg_test.o:  peerview_address_request_msg_test.c 
	$(COMPILE) -DSTANDALONE -o peerview_address_request_msg_test.o -c $(srcdir)/peerview_address_request_msg_test.c

peerview_address_assign_msg_test_SOURCES	     = peerview_address_assign_msg_test.c unittest_jxta_func.c
peerview_address_assign_msg_test.o:  peerview_address_assign_msg_test.c 
	$(COMPILE) -DSTANDALONE -o peerview_address_assign_msg_test.o -c $(srcdir)/peerview_address_assign_msg_test.c

traffic_shaping_test_SOURCES	     = traffic_shaping_test.c unittest_jxta_func.c
traffic_shaping_test.o:  traffic_shaping_test.c 
	$(COMPILE) -DSTANDALONE -o traffic_shaping_test.o -c $(srcdir)/traffic_shaping_test.c

cm_test_SOURCES		     = cm_test.c
cm_test.o:  cm_test.c
	$(COMPILE) -DSTANDALONE -o cm_test.o -c $(srcdir)/cm_test.c

monitor_msg_test_SOURCES	     = monitor_msg_test.c unittest_jxta_func.c
monitor_msg_test.o:  monitor_msg_test.c 
	$(COMPILE) -DSTANDALONE -o monitor_msg_test.o -c $(srcdir)/monitor_msg_test.c

unit_test_runner_SOURCES = unit_test_runner.c		    \
			   unittest_jxta_func.c

unit_test_runner_extra_obj = jstring_test_comp.o	    \
			   jxta_bytevector_test_comp.o	    \
			   jxta_vector_test_comp.o	    \
			   jxta_xml_util_test_comp.o	    \
			   jxtaobject_test_comp.o	    \
			   jxta_id_test_comp.o		    \
			   jxta_hash_test_comp.o	    \
			   jxta_objecthash_test_comp.o	    \
			   excep_test_comp.o		    \
			   dummypg_test_comp.o		    \
			   msg_test_comp.o		    \
			   pga_adv_test_comp.o		    \
			   pa_adv_test_comp.o		    \
			   dq_adv_test_comp.o               \
			   apa_adv_test_comp.o               \
			   route_adv_test_comp.o               \
			   lease_msg_test_comp.o            \
			   rdv_lease_options_test_comp.o            \
			   peerview_ping_msg_test_comp.o            \
			   peerview_pong_msg_test_comp.o            \
			   peerview_address_request_msg_test_comp.o            \
			   peerview_address_assign_msg_test_comp.o            \
			   traffic_shaping_test_comp.o \
			   rdv_diffusion_msg_test_comp.o

unit_test_runner_DEPENDENCIES = $(unit_test_runner_extra_obj)

unit_test_runner_LDADD = $(unit_test_runner_extra_obj) $(LDADD)

jstring_test_comp.o:  jstring_test.c
	$(COMPILE) -o jstring_test_comp.o -c $(srcdir)/jstring_test.c
jxta_bytevector_test_comp.o:  jxta_bytevector_test.c
	$(COMPILE) -o jxta_bytevector_test_comp.o -c $(srcdir)/jxta_bytevector_test.c
jxta_vector_test_comp.o:  jxta_vector_test.c
	$(COMPILE) -o jxta_vector_test_comp.o -c $(srcdir)/jxta_vector_test.c
jxta_xml_util_test_comp.o:  jxta_xml_util_test.c
	$(COMPILE) -o jxta_xml_util_test_comp.o -c $(srcdir)/jxta_xml_util_test.c
jxtaobject_test_comp.o:  jxtaobject_test.c
	$(COMPILE) -o jxtaobject_test_comp.o -c $(srcdir)/jxtaobject_test.c
jxta_id_test_comp.o:  jxta_id_test.c
	$(COMPILE) -o jxta_id_test_comp.o -c $(srcdir)/jxta_id_test.c
jxta_hash_test_comp.o:	jxta_hash_test.c
	$(COMPILE) -o jxta_hash_test_comp.o -c $(srcdir)/jxta_hash_test.c
jxta_objecthash_test_comp.o:  jxta_objecthash_test.c
	$(COMPILE) -o jxta_objecthash_test_comp.o -c $(srcdir)/jxta_objecthash_test.c
excep_test_comp.o:  excep_test.c
	$(COMPILE) -o excep_test_comp.o -c $(srcdir)/excep_test.c
dummypg_test_comp.o:  dummypg_test.c
	$(COMPILE) -o dummypg_test_comp.o -c $(srcdir)/dummypg_test.c
pga_adv_test_comp.o:  pga_adv_test.c
	$(COMPILE) -o pga_adv_test_comp.o -c $(srcdir)/pga_adv_test.c
apa_adv_test_comp.o:  apa_adv_test.c
	$(COMPILE) -o apa_adv_test_comp.o -c $(srcdir)/apa_adv_test.c
route_adv_test_comp.o:  route_adv_test.c
	$(COMPILE) -o route_adv_test_comp.o -c $(srcdir)/route_adv_test.c
pa_adv_test_comp.o:  pa_adv_test.c
	$(COMPILE) -o pa_adv_test_comp.o -c $(srcdir)/pa_adv_test.c
rdv_diffusion_msg_test_comp.o:  rdv_diffusion_msg_test.c
	$(COMPILE) -o rdv_diffusion_msg_test_comp.o -c $(srcdir)/rdv_diffusion_msg_test.c
lease_msg_test_comp.o:  lease_msg_test.c
	$(COMPILE) -o lease_msg_test_comp.o -c $(srcdir)/lease_msg_test.c
msg_test_comp.o:  msg_test.c
	$(COMPILE) -o msg_test_comp.o -c $(srcdir)/msg_test.c
dq_adv_test_comp.o:  dq_adv_test.c
	$(COMPILE) -o dq_adv_test_comp.o -c $(srcdir)/dq_adv_test.c
rdv_lease_options_test_comp.o:  rdv_lease_options_test.c
	$(COMPILE) -o rdv_lease_options_test_comp.o -c $(srcdir)/rdv_lease_options_test.c
endpoint_msg_test_comp.o:  endpoint_msg_test.c
	$(COMPILE) -o endpoint_msg_test_comp.o -c $(srcdir)/endpoint_msg_test.c
peerview_ping_msg_test_comp.o:  peerview_ping_msg_test.c
	$(COMPILE) -o peerview_ping_msg_test_comp.o -c $(srcdir)/peerview_ping_msg_test.c
peerview_pong_msg_test_comp.o:  peerview_pong_msg_test.c
	$(COMPILE) -o peerview_pong_msg_test_comp.o -c $(srcdir)/peerview_pong_msg_test.c
peerview_address_request_msg_test_comp.o:  peerview_address_request_msg_test.c
	$(COMPILE) -o peerview_address_request_msg_test_comp.o -c $(srcdir)/peerview_address_request_msg_test.c
peerview_address_assign_msg_test_comp.o:  peerview_address_assign_msg_test.c
	$(COMPILE) -o peerview_address_assign_msg_test_comp.o -c $(srcdir)/peerview_address_assign_msg_test.c
traffic_shaping_test_comp.o:  traffic_shaping_test.c
	$(COMPILE) -o traffic_shaping_test_comp.o -c $(srcdir)/traffic_shaping_test.c
