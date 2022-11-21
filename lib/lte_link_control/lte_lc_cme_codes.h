/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** CME Error codes as defined by 3GPP TS 27.007 version 14.10.0
 * Mobile equipment errors
*/

#define CME_CODES \
	X(0, "phone failure") \
	X(1, "no connection to phone") \
	X(2, "phone-adaptor link reserved") \
	X(3, "operation not allowed") \
	X(4, "operation not supported") \
	X(5, "PH-SIM PIN required") \
	X(6, "PH-FSIM PIN required") \
	X(7, "PH-FSIM PUK required") \
	X(10, "SIM not inserted") \
	X(11, "SIM PIN required") \
	X(12, "SIM PUK required") \
	X(13, "SIM failure") \
	X(14, "SIM busy") \
	X(15, "SIM wrong") \
	X(16, "incorrect password") \
	X(17, "SIM PIN2 required") \
	X(18, "SIM PUK2 required") \
	X(20, "memory full") \
	X(21, "invalid index") \
	X(22, "not found") \
	X(23, "memory failure") \
	X(24, "text string too long") \
	X(25, "invalid characters in text string") \
	X(26, "dial string too long") \
	X(27, "invalid characters in dial string") \
	X(30, "no network service") \
	X(31, "network timeout") \
	X(32, "network not allowed - emergency calls only") \
	X(40, "network personalization PIN required") \
	X(41, "network personalization PUK required") \
	X(42, "network subset personalization PIN required") \
	X(43, "network subset personalization PUK required") \
	X(44, "service provider personalization PIN required") \
	X(45, "service provider personalization PUK required") \
	X(46, "corporate personalization PIN required") \
	X(47, "corporate personalization PUK required") \
	X(48, "hidden key required") \
	X(49, "EAP method not supported") \
	X(50, "Incorrect parameters") \
	X(51, "command implemented but currently disabled") \
	X(52, "command aborted by user") \
	X(53, "not attached to network due to MT functionality restrictions") \
	X(54, "modem not allowed - MT restricted to emergency calls only") \
	X(55, "operation not allowed because of MT functionality restrictions") \
	X(56, "fixed dial number only allowed - called number is not a fixed dial number (refer 3GPP TS 22.101 [147])") \
	X(57, "temporarily out of service due to other MT usage") \
	X(58, "language/alphabet not supported") \
	X(59, "unexpected data value") \
	X(60, "system failure") \
	X(61, "data missing") \
	X(62, "call barred") \
	X(63, "message waiting indication subscription failure") \
	X(100, "unknown") \
	X(103, "Illegal MS") \
	X(106, "Illegal ME") \
	X(107, "GPRS services not allowed") \
	X(108, "GPRS services and non-GPRS services not allowed") \
	X(111, "PLMN not allowed") \
	X(112, "Location area not allowed") \
	X(113, "Roaming not allowed in this location area") \
	X(114, "GPRS services not allowed in this PLMN") \
	X(115, "No Suitable Cells In Location Area") \
	X(122, "Congestion") \
	X(125, "Not authorized for this CSG") \
	X(126, "insufficient resources") \
	X(127, "missing or unknown APN") \
	X(128, "unknown PDP address or PDP type") \
	X(129, "user authentication failed") \
	X(130, "activation rejected by GGSN, Serving GW or PDN GW") \
	X(131, "activation rejected, unspecified") \
	X(132, "service option not supported") \
	X(133, "requested service option not subscribed") \
	X(134, "service option temporarily out of order") \
	X(140, "feature not supported") \
	X(141, "semantic error in the TFT operation") \
	X(142, "syntactical error in the TFT operation") \
	X(143, "unknown PDP context") \
	X(144, "semantic errors in packet filter(s)") \
	X(145, "syntactical errors in packet filter(s)") \
	X(146, "PDP context without TFT already activated") \
	X(148, "unspecified GPRS error") \
	X(149, "PDP authentication failure") \
	X(150, "invalid mobile class") \
	X(151, "VBS/VGCS not supported by the network") \
	X(152, "No service subscription on SIM") \
	X(153, "No subscription for group ID") \
	X(154, "Group Id not activated on SIM") \
	X(155, "No matching notification") \
	X(156, "VBS/VGCS call already present") \
	X(157, "Congestion") \
	X(158, "Network failure") \
	X(159, "Uplink busy") \
	X(160, "No access rights for SIM file") \
	X(161, "No subscription for priority") \
	X(162, "operation not applicable or not possible") \
	X(163, "Group Id prefixes not supported") \
	X(164, "Group Id prefixes not usable for VBS") \
	X(165, "Group Id prefix value invalid") \
	X(171, "Last PDN disconnection not allowed") \
	X(172, "Semantically incorrect message") \
	X(173, "Mandatory information element error") \
	X(174, "Information element non-existent or not implemented") \
	X(175, "Conditional IE error") \
	X(176, "Protocol error, unspecified") \
	X(177, "Operator Determined Barring") \
	X(178, "maximum number of PDP contexts reached") \
	X(179, "requested APN not supported in current RAT and PLMN combination") \
	X(180, "request rejected, Bearer Control Mode violation") \
	X(181, "unsupported QCI value") \
	X(182, "user data transmission via control plane is congested")

/* prefix, code, str */
#define CME_CODES_NORDIC \
	X("AT%XSUDO", 513, "Public key not found") \
	X("AT%JWT", 513, "Key not found") \
	X("", 513, "Not found") \
	X("AT%CMNG", 514, "No access")  \
	X("AT%KEYGEN", 514, "Not allowed") \
	X("AT%JWT", 514, "Could not read key") \
	X("", 515, "Memory full") \
	X("", 516, "Radio connection is active") \
	X("", 517, "modem not activated") \
	X("", 518, "Not allowed in active state") \
	X("", 519, "Already exists") \
	X("AT%XSUDO", 520, "Authentication failed") \
	X("AT%XPMNG", 520, "For write: Already exists")  \
	X("", 521, "PLMN search interrupted, partial results") \
	X("", 522, "Band configuration not valid for selected mode") \
	X("", 523, "Key generation failed") \
	X("", 525, "Error in JWT creation") \
	X("", 527, "Invalid content") \
	X("", 528, "Not allowed in Power off warning state")
