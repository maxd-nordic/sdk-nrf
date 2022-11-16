/* Network error code notification error codes */

/* 3GPP TS 24.301 version 17.8.0 */

/*EMM*/
0b00000010 IMSI unknown in HSS
0b00000011 Illegal UE
0b00000101 IMEI not accepted
0b00000110 Illegal ME
0b00000111 EPS services not allowed
0b00001000 EPS services and non-EPS services not allowed
0b00001001 UE identity cannot be derived by the network
0b00001010 Implicitly detached
0b00001011 PLMN not allowed
0b00001100 Tracking Area not allowed
0b00001101 Roaming not allowed in this tracking area
0b00001110 EPS services not allowed in this PLMN
0b00001111 No Suitable Cells In tracking area
0b00010000 MSC temporarily not reachable
0b00010001 Network failure
0b00010010 CS domain not available
0b00010011 ESM failure
0b00010100 MAC failure
0b00010101 Synch failure
0b00010110 Congestion
0b00010111 UE security capabilities mismatch
0b00011000 Security mode rejected, unspecified
0b00011001 Not authorized for this CSG
0b00011010 Non-EPS authentication unacceptable
0b00011111 Redirection to 5GCN required
0b00100011 Requested service option not authorized in this PLMN
0b00100111 CS service temporarily not available
0b00101000 No EPS bearer context activated
0b00101010 Severe network failure
0b01001110 PLMN not allowed to operate at the present UE location
0b01011111 Semantically incorrect message
0b01100000 Invalid mandatory information
0b01100001 Message type non-existent or not implemented
0b01100010 Message type not compatible with the protocol state
0b01100011 Information element non-existent or not implemented
0b01100100 Conditional IE error
0b01100101 Message not compatible with the protocol state
0b01101111 Protocol error, unspecified

/*ESM*/
0b00001000 Operator Determined Barring
0b00011010 Insufficient resources
0b00011011 Missing or unknown APN
0b00011100 Unknown PDN type
0b00011101 User authentication failed
0b00011110 Request rejected by Serving GW or PDN GW
0b00011111 Request rejected, unspecified
0b00100000 Service option not supported
0b00100001 Requested service option not subscribed
0b00100010 Service option temporarily out of order
0b00100011 PTI already in use
0b00100100 Regular deactivation
0b00100101 EPS QoS not accepted
0b00100110 Network failure
0b00100111 Reactivation requested
0b00101001 Semantic error in the TFT operation
0b00101010 Syntactical error in the TFT operation
0b00101011 Invalid EPS bearer identity
0b00101100 Semantic errors in packet filter(s)
0b00101101 Syntactical errors in packet filter(s)
0b00101110 Unused (see NOTE 2)
0b00101111 PTI mismatch
0b00110001 Last PDN disconnection not allowed
0b00110010 PDN type IPv4 only allowed
0b00110011 PDN type IPv6 only allowed
0b00111001 PDN type IPv4v6 only allowed
0b00111010 PDN type non IP only allowed
0b00110100 Single address bearers only allowed
0b00110101 ESM information not received
0b00110110 PDN connection does not exist
0b00110111 Multiple PDN connections for a given APN not allowed
0b00111000 Collision with network initiated request
0b00111011 Unsupported QCI value
0b00111100 Bearer handling not supported
0b00111101 PDN type Ethernet only allowed
0b01000001 Maximum number of EPS bearers reached
0b01000010 Requested APN not supported in current RAT and PLMN combination
0b01010001 Invalid PTI value
0b01011111 Semantically incorrect message
0b01100000 Invalid mandatory information
0b01100001 Message type non-existent or not implemented
0b01100010 Message type not compatible with the protocol state
0b01100011 Information element non-existent or not implemented
0b01100100 Conditional IE error
0b01100101 Message not compatible with the protocol state
0b01101111 Protocol error, unspecified
0b01110000 APN restriction value incompatible with active EPS bearer context
0b01110001 Multiple accesses to a PDN connection not allowed