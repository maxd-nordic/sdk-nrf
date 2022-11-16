/* Message Service Errors */

#define CMS_NORDIC_NOT_FOUND 513
#define CMS_NORDIC_NOT_ALLOWED 514
#define CMS_NORDIC_NO_MEM 515

/* 3GPP TS 24.011 version 14.3.0 */
E-1: CP-cause definition.
Cause no. 17: "Network failure".
Cause no. 22: "Congestion".
Cause no. 81: "Invalid Transaction Identifier".
Cause no. 95: "Semantically incorrect message".
Cause no. 96: "Invalid mandatory information".
Cause no. 97: "Message type non-existent or not implemented".
Cause no. 98: "Message not compatible with short message protocol state".
Cause no. 99: "Information element non-existent or not implemented".
Cause no. 111: "Protocol error, unspecified".

E-2: RP-cause definition mobile originating SM-transfer.
Cause no. 1: "Unassigned (unallocated) number".
Cause no. 8: "Operator determined barring".
Cause no. 10: "Call barred".
Cause no. 21: "Short message transfer rejected".
Cause no. 27: "Destination out of service".
Cause no. 28: "Unidentified subscriber".
Cause no. 29: "Facility rejected".
Cause no. 30: "Unknown subscriber".
Cause no. 38: "Network out of order".
Cause no. 41: "Temporary failure".
Cause no. 42: "Congestion".
Cause no. 47: "Resources unavailable, unspecified".
Cause no. 50: "Requested facility not subscribed".
Cause no. 69: "Requested facility not implemented".
Cause no. 81: "Invalid short message transfer reference value".
Cause no. 95: "Invalid message, unspecified".
Cause no. 96: "Invalid mandatory information".
Cause no. 97: "Message type non-existent or not implemented".
Cause no. 98: "Message not compatible with short message protocol state".
Cause no. 99: "Information element non-existent or not implemented".
Cause no. 111: "Protocol error, unspecified".
Cause no. 127: "Interworking, unspecified".

E-3: RP-cause definition mobile terminating SM-transfer.
Cause no. 22: "Memory capacity exceeded".
Cause no. 81: "Invalid short message reference value".
Cause no. 95: "Invalid message, unspecified".
Cause no. 96: "Invalid mandatory information".
Cause no. 97: "Message type non-existent or not implemented".
Cause no. 98: "Message not compatible with short message protocol state".
Cause no. 99: "Information element non-existent or not implemented".
Cause no. 111: "Protocol error, unspecified".

E-4: RP-Cause definition memory available notification.
Cause no. 30: "Unknown Subscriber".
Cause no. 38: "Network out of order".
Cause no. 41: "Temporary failure".
Cause no. 42: "Congestion".
Cause no. 47: "Resources unavailable, unspecified".
Cause no. 69: "Requested facility not implemented".
Cause no. 95: "Invalid message, unspecified".
Cause no. 96: "Invalid mandatory information".
Cause no. 97: "Message type non-existent or not implemented".
Cause no. 98: "Message not compatible with short message protocol state".
Cause no. 99: "Information element non-existent or not implemented".
Cause no. 111: "Protocol error, unspecified".
Cause no. 127: "Interworking, unspecified".


/* 3GPP TS 23.040 version 14.0.0 */
//0x00 - 7F Reserved
//0x80 - 8F TP-PID errors
0x80 Telematic interworking not supported
0x81 Short message Type 0 not supported
0x82 Cannot replace short message
//0x83 - 8E Reserved
0x8F Unspecified TP-PID error
//0x90 - 9F TP-DCS errors
0x90 Data coding scheme (alphabet) not supported
0x91 Message class not supported
//0x92 - 9E Reserved
0x9F Unspecified TP-DCS error
//0xA0 - AF TP-Command Errors
0xA0 Command cannot be actioned
0xA1 Command unsupported
//0xA2 - AE Reserved
0xAF Unspecified TP-Command error
0xB0 TPDU not supported
//0xB1 - BF Reserved
0xC0 SC busy
0xC1 No SC subscription
0xC2 SC system failure
0xC3 Invalid SME address
0xC4 Destination SME barred
0xC5 SM Rejected-Duplicate SM
0xC6 TP-VPF not supported
0xC7 TP-VP not supported
//0xC8 - CF Reserved
0xD0 (U)SIM SMS storage full
0xD1 No SMS storage capability in (U)SIM
0xD2 Error in MS
0xD3 Memory Capacity Exceeded
0xD4 (U)SIM Application Toolkit Busy
0xD5 (U)SIM data download error
//0xD6 - DF Reserved
//0xE0 - FE Values specific to an application
0xFF Unspecified error cause

/* 3GPP TS 27.005 version 14.0.0 */
300 ME failure
301 SMS service of ME reserved
302 operation not allowed
303 operation not supported
304 invalid PDU mode parameter
305 invalid text mode parameter
310 (U)SIM not inserted
311 (U)SIM PIN required
312 PH-(U)SIM PIN required
313 (U)SIM failure
314 (U)SIM busy
315 (U)SIM wrong
316 (U)SIM PUK required
317 (U)SIM PIN2 required
318 (U)SIM PUK2 required
320 memory failure
321 invalid memory index
322 memory full
330 SMSC address unknown
331 no network service
332 network timeout
340 no +CNMA acknowledgement expected
500 unknown error