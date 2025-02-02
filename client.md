# Client Program Flowchart

```mermaid
graph TD
    A[Start Client Program] --> B[Create Socket]
    B --> C[Initialize Server Address]
    C --> D[Connect to Server]
    D -->|Connection Failed| E[Exit with Error]
    D -->|Connection Successful| F[Check Command-Line Argument]
    
    F -->|Argument Provided| G[Send File to Server]
    F -->|No Argument Provided| H[Receive File from Server]

    G --> G1[Open File]
    G1 -->|File Not Found| E1[Exit with Error]
    G1 --> G2[Send File Name]
    G2 --> G3[Send File Size]
    G3 --> G4[Send File Content in Chunks]
    G4 --> G5[Close File and Print Success Message]

    H --> H1[Receive File Name]
    H1 -->|Error Receiving Name| E2[Exit with Error]
    H1 --> H2[Receive File Size]
    H2 -->|Error Receiving Size| E3[Exit with Error]
    H2 --> H3[Create File to Write]
    H3 -->|Error Creating File| E4[Exit with Error]
    H3 --> H4[Receive File Content in Chunks]
    H4 --> H5[Write Content to File]
    H5 --> H6[Close File and Print Success Message]

    G5 --> I[Close Socket]
    H6 --> I[Close Socket]
    I --> J[End Program]
