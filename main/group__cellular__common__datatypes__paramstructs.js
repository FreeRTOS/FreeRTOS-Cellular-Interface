var group__cellular__common__datatypes__paramstructs =
[
    [ "CellularAtReq_t", "struct_cellular_at_req__t.html", [
      [ "pAtCmd", "struct_cellular_at_req__t.html#a78daaea1eb323c2e788003c009d504ec", null ],
      [ "atCmdType", "struct_cellular_at_req__t.html#a2aeb0ea37a6746a2c4b73660716b6423", null ],
      [ "pAtRspPrefix", "struct_cellular_at_req__t.html#ac312650455dc906bacd0795578860591", null ],
      [ "respCallback", "struct_cellular_at_req__t.html#a72367139b2a234ecaf289df223244c7d", null ],
      [ "pData", "struct_cellular_at_req__t.html#abc4ae9d3ea15040c79fe8fea960f8cf0", null ],
      [ "dataLen", "struct_cellular_at_req__t.html#a9743ccbd2e82f2c0f2e6f925d4fdb996", null ]
    ] ],
    [ "CellularAtDataReq_t", "struct_cellular_at_data_req__t.html", [
      [ "pData", "struct_cellular_at_data_req__t.html#a410fec7ac962fd6c98eff0f76a5c104a", null ],
      [ "dataLen", "struct_cellular_at_data_req__t.html#a96f6945df2a1764084e5b8d6f065654c", null ],
      [ "pSentDataLength", "struct_cellular_at_data_req__t.html#a2e7f7c417c9e5fa5aaa8fe8046a4cd73", null ],
      [ "pEndPattern", "struct_cellular_at_data_req__t.html#ac8b1e457e6e8b0846013b60472098ca1", null ],
      [ "endPatternLen", "struct_cellular_at_data_req__t.html#ac76c2a8fdb726a9cf402decea3a2107d", null ]
    ] ],
    [ "CellularAtParseTokenMap_t", "struct_cellular_at_parse_token_map__t.html", [
      [ "pStrValue", "struct_cellular_at_parse_token_map__t.html#a1ff0f0741d686d320accfab892b63623", null ],
      [ "parserFunc", "struct_cellular_at_parse_token_map__t.html#a86ed2448ea66d09fe27bdd08b2940798", null ]
    ] ],
    [ "CellularSocketContext_t", "struct_cellular_socket_context__t.html", [
      [ "contextId", "struct_cellular_socket_context__t.html#aac523065b94c9e4140f6e7555a54438d", null ],
      [ "socketId", "struct_cellular_socket_context__t.html#a6f05ae7bf8110bf0183f4303dcc5520d", null ],
      [ "socketState", "struct_cellular_socket_context__t.html#ac79a634a46ea79903e24a80bbd117075", null ],
      [ "socketType", "struct_cellular_socket_context__t.html#a22dc063f6505be86edbe2ee3f25f2a87", null ],
      [ "socketDomain", "struct_cellular_socket_context__t.html#a9fc5d568b18b8d74ede134040ca8d132", null ],
      [ "socketProtocol", "struct_cellular_socket_context__t.html#ad2b91a7e33e2dedafee89add7e1fe1ba", null ],
      [ "localIpAddress", "struct_cellular_socket_context__t.html#ae5704e11a60a710fe747fd17a53b4fe9", null ],
      [ "localPort", "struct_cellular_socket_context__t.html#aba39bc30c8013db7d41a62865ca309b0", null ],
      [ "dataMode", "struct_cellular_socket_context__t.html#a8293b82cb491875420a8477f75200aef", null ],
      [ "sendTimeoutMs", "struct_cellular_socket_context__t.html#a74b7b7d17299c44ab0926c76bc0c1e14", null ],
      [ "recvTimeoutMs", "struct_cellular_socket_context__t.html#a4dd36a5e9b741068b95be7af773a9588", null ],
      [ "remoteSocketAddress", "struct_cellular_socket_context__t.html#af2941649e911fbd45055dfbbe371f69d", null ],
      [ "dataReadyCallback", "struct_cellular_socket_context__t.html#a18ff2d5b31108942fa3234101dce9da8", null ],
      [ "pDataReadyCallbackContext", "struct_cellular_socket_context__t.html#aebe3d8b8e56ff2ab4b6b89a39af7cda9", null ],
      [ "openCallback", "struct_cellular_socket_context__t.html#a73740e7a823925884ff049fbad82c889", null ],
      [ "pOpenCallbackContext", "struct_cellular_socket_context__t.html#ac66008a4d4a37fa7e2d0c0d6dcd0690d", null ],
      [ "closedCallback", "struct_cellular_socket_context__t.html#abad9d60861ee81a48cb0093a36290558", null ],
      [ "pClosedCallbackContext", "struct_cellular_socket_context__t.html#ab88a308e2f7c8d181ed9fe1e622498ed", null ],
      [ "pModemData", "struct_cellular_socket_context__t.html#aff1aa3affda1c42912533127beda1d63", null ]
    ] ],
    [ "CellularTokenTable_t", "struct_cellular_token_table__t.html", [
      [ "pCellularUrcHandlerTable", "struct_cellular_token_table__t.html#a478085a6a0a03103e981682d3eace986", null ],
      [ "cellularPrefixToParserMapSize", "struct_cellular_token_table__t.html#a8830849fb19a562da497f71e0b8136f5", null ],
      [ "pCellularSrcTokenErrorTable", "struct_cellular_token_table__t.html#ada0e4e53923e52c07738e58a430376cc", null ],
      [ "cellularSrcTokenErrorTableSize", "struct_cellular_token_table__t.html#a23fc0f804ae16eb670f8fd7a559e65f1", null ],
      [ "pCellularSrcTokenSuccessTable", "struct_cellular_token_table__t.html#ac4353497c33823d28fd5bd5a871a06ee", null ],
      [ "cellularSrcTokenSuccessTableSize", "struct_cellular_token_table__t.html#ad878b89414daefb2a3653cc3ac502aca", null ],
      [ "pCellularUrcTokenWoPrefixTable", "struct_cellular_token_table__t.html#a3e4b73326ec6ac61e5891c60bef0914e", null ],
      [ "cellularUrcTokenWoPrefixTableSize", "struct_cellular_token_table__t.html#a6472ffee2e213e3bd1b2e6065ac1f2df", null ],
      [ "pCellularSrcExtraTokenSuccessTable", "struct_cellular_token_table__t.html#a93c94ab9326198ade445dcff73fe9adb", null ],
      [ "cellularSrcExtraTokenSuccessTableSize", "struct_cellular_token_table__t.html#a3cc01c60dba0c69e496ade6f6805256f", null ]
    ] ]
];