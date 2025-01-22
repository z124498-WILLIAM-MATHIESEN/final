# Server Program Flowchart

```mermaid
graph TD
    A[Start Server] --> B[Create Socket]
    B --> C[Set Socket Options]
    C --> D[Bind to Address]
    D --> E[Listen for Connections]
    E --> F[Accept Connection]
    F --> G[Spawn Thread for Client]
    G --> H[Client Handler]
    H --> I[Receive File Metadata]
    I --> J[Validate File Size]
    J -->|Invalid| K[Reject Connection]
    J -->|Valid| L[Save File to Disk]
    L --> M[Broadcast to Other Clients]
    M --> N[Loop Through Clients]
    N --> O[Send Metadata and Content]
    O --> P[Close Connections]
    P --> Q[End]
