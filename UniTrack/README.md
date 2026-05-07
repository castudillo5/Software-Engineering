# UniTrack — Degree Progress System
 
A C++ backend with HTML/JavaScript frontend for tracking degree progress at MSU Texas. Course catalog comes from a CSV file, student state is persisted to text files, and the frontend talks to the backend over `localhost`.
 
---
 
## Requirements
 
- **Visual Studio 2019 or later** with the **Desktop Development with C++** workload installed
- **Windows 10 or 11**
- C++17 compiler (standard with VS2019+)
---
 
## Getting the Project
 
1. Go to https://download-directory.github.io/
2. Paste this URL into the input box:
   ```
   https://github.com/castudillo5/Software-Engineering/tree/main/UniTrack
   ```
3. Click **Download** — you'll get a zip of just the UniTrack folder
4. Extract the zip somewhere convenient (e.g., your Desktop)
---
 
## Project Structure
 
After extracting, your folder should look like this:
 
```
UniTrack/
├── UniTrack.sln           ← open this in Visual Studio
├── UniTrack.vcxproj
├── README.md
├── .gitignore
├── src/                   ← C++ source code
│   ├── Server.cpp         ← entry point
│   ├── Student.cpp / .h
│   ├── AcademicRecord.cpp / .h
│   ├── Course.cpp / .h
│   ├── Section.cpp / .h
│   ├── Alert.cpp / .h
│   ├── DegreeAudit.cpp / .h
│   ├── DegreePlan.cpp / .h
│   ├── Structs.cpp / .h
│   └── httplib.h          ← header-only HTTP server library
├── temp/                  ← frontend HTML/JS — served at localhost:8080
│   ├── home.html
│   ├── schedule.html
│   ├── registration.html
│   ├── alerts.html
│   ├── api.js
│   └── nav.js
└── data/                  ← CSV catalog and persisted student data
    ├── Schedule_with_Credits.csv
    └── student_data.txt
```
 
---
 
## Setup (One-Time)
 
### 1. Open the solution
 
Double-click `UniTrack.sln` to open it in Visual Studio.
 
### 2. Set the Working Directory
 
**This is the most important configuration step.** The server reads files using paths relative to the project root, not the executable. If you skip this, you'll see "Catalog loaded: 0 courses" when running.
 
1. In Solution Explorer, right-click the **UniTrack** project → **Properties**
2. Navigate to **Configuration Properties → Debugging**
3. Set **Working Directory** to: `$(ProjectDir)`
4. Click **Apply** and **OK**
### 3. Confirm build settings
 
Make sure the toolbar shows:
- Configuration: **Debug**
- Platform: **x64**
### 4. Allow through Windows Firewall
 
The first time you run, Windows may prompt you to allow the executable through the firewall. Click **Allow access** for both Private and Public networks.
 
---
 
## Running the Project
 
1. Press **Ctrl+F5** (Start Without Debugging)
2. A console window opens showing:
   ```
   Catalog loaded: 124 courses
   Student loaded: yes
     Name: Alex Johnson
     Major: Computer Science
     Records: 6
   Server running at http://localhost:8080/
   ```
3. Your default browser opens to `http://localhost:8080/home.html` automatically
If the browser doesn't open, navigate to that URL manually.
