# EcoTrack – Smart Waste Collection & Recycling Tracker

## Problem
Waste bins can overflow when collection is not based on actual fill levels, while students have little incentive to dispose of waste correctly.

## Solution
EcoTrack tracks waste-bin capacity, records disposal quantities, generates reward points for responsible disposal, and records collection events.

## Features
- User and collector records
- Multiple waste categories
- Bin capacity and fill-level tracking
- Overflow prevention
- Automatic FULL/ACTIVE status
- Waste collection history
- Rule-based reward points
- Sustainability report showing collected quantity and bins above 80%

## Tech
C++, SQLite, SQL, DBMS

## Build
Linux/macOS:
`g++ main.cpp -lsqlite3 -o ecotrack`
`./ecotrack`

Windows MinGW (if SQLite development files are installed):
`g++ main.cpp -lsqlite3 -o ecotrack.exe`
`ecotrack.exe`

The program automatically creates `ecotrack.db` and inserts sample data.

## Demo IDs
Users:
1 = Student A
2 = Collector B

Sample bins:
Block A, Canteen, Library



