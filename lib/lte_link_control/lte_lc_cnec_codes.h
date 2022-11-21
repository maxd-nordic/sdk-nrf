/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Network error code notification error codes */

/* 3GPP TS 24.301 version 17.8.0 */

/* EMM */
#define CNEC_EMM_CODES \
	X(2, "IMSI unknown in HSS") \
	X(3, "Illegal UE") \
	X(5, "IMEI not accepted") \
	X(6, "Illegal ME") \
	X(7, "EPS services not allowed") \
	X(8, "EPS services and non-EPS services not allowed") \
	X(9, "UE identity cannot be derived by the network") \
	X(10, "Implicitly detached") \
	X(11, "PLMN not allowed") \
	X(12, "Tracking Area not allowed") \
	X(13, "Roaming not allowed in this tracking area") \
	X(14, "EPS services not allowed in this PLMN") \
	X(15, "No Suitable Cells In tracking area") \
	X(16, "MSC temporarily not reachable") \
	X(17, "Network failure") \
	X(18, "CS domain not available") \
	X(19, "ESM failure") \
	X(20, "MAC failure") \
	X(21, "Synch failure") \
	X(22, "Congestion") \
	X(23, "UE security capabilities mismatch") \
	X(24, "Security mode rejected, unspecified") \
	X(25, "Not authorized for this CSG") \
	X(26, "Non-EPS authentication unacceptable") \
	X(31, "Redirection to 5GCN required") \
	X(35, "Requested service option not authorized in this PLMN") \
	X(39, "CS service temporarily not available") \
	X(40, "No EPS bearer context activated") \
	X(42, "Severe network failure") \
	X(78, "PLMN not allowed to operate at the present UE location") \
	X(95, "Semantically incorrect message") \
	X(96, "Invalid mandatory information") \
	X(97, "Message type non-existent or not implemented") \
	X(98, "Message type not compatible with the protocol state") \
	X(99, "Information element non-existent or not implemented") \
	X(100, "Conditional IE error") \
	X(101, "Message not compatible with the protocol state") \
	X(111, "Protocol error, unspecified")

/* ESM */
#define CNEC_ESM_CODES \
	X(8, "Operator Determined Barring") \
	X(26, "Insufficient resources") \
	X(27, "Missing or unknown APN") \
	X(28, "Unknown PDN type") \
	X(29, "User authentication failed") \
	X(30, "Request rejected by Serving GW or PDN GW") \
	X(31, "Request rejected, unspecified") \
	X(32, "Service option not supported") \
	X(33, "Requested service option not subscribed") \
	X(34, "Service option temporarily out of order") \
	X(35, "PTI already in use") \
	X(36, "Regular deactivation") \
	X(37, "EPS QoS not accepted") \
	X(38, "Network failure") \
	X(39, "Reactivation requested") \
	X(41, "Semantic error in the TFT operation") \
	X(42, "Syntactical error in the TFT operation") \
	X(43, "Invalid EPS bearer identity") \
	X(44, "Semantic errors in packet filter(s)") \
	X(45, "Syntactical errors in packet filter(s)") \
	X(47, "PTI mismatch") \
	X(49, "Last PDN disconnection not allowed") \
	X(50, "PDN type IPv4 only allowed") \
	X(51, "PDN type IPv6 only allowed") \
	X(57, "PDN type IPv4v6 only allowed") \
	X(58, "PDN type non IP only allowed") \
	X(52, "Single address bearers only allowed") \
	X(53, "ESM information not received") \
	X(54, "PDN connection does not exist") \
	X(55, "Multiple PDN connections for a given APN not allowed") \
	X(56, "Collision with network initiated request") \
	X(59, "Unsupported QCI value") \
	X(60, "Bearer handling not supported") \
	X(61, "PDN type Ethernet only allowed") \
	X(65, "Maximum number of EPS bearers reached") \
	X(66, "Requested APN not supported in current RAT and PLMN combination") \
	X(81, "Invalid PTI value") \
	X(95, "Semantically incorrect message") \
	X(96, "Invalid mandatory information") \
	X(97, "Message type non-existent or not implemented") \
	X(98, "Message type not compatible with the protocol state") \
	X(99, "Information element non-existent or not implemented") \
	X(100, "Conditional IE error") \
	X(101, "Message not compatible with the protocol state") \
	X(111, "Protocol error, unspecified") \
	X(112, "APN restriction value incompatible with active EPS bearer context") \
	X(113, "Multiple accesses to a PDN connection not allowed")
