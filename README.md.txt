# Hospital Patient Appointment System

## 📖 Description
This is a simple console-based hospital appointment management system written in C.  
It demonstrates the use of four core data structures:
- Singly Linked List (patients)
- Binary Search Tree (doctors)
- Circular Queue (appointment requests)
- Stack (undo last appointment)

## ✨ Features
- Add/Delete patients
- Add/List doctors
- Queue appointment requests
- Process requests and schedule appointments
- Undo the most recent appointment
- Display all scheduled appointments with patient & doctor names

## 🛠️ How to Compile and Run
Make sure you have a C compiler installed (e.g., GCC).

```bash
gcc -Wall -o hospital hospital_by_name.c
./hospital

output:
=== Hospital Simple Menu ===
1  - Add patient
2  - Delete patient
3  - List patients
4  - Add doctor
5  - List doctors
6  - Enqueue appointment request
7  - Show request queue
8  - Process next request (schedule)
9  - Undo last scheduled appointment
10 - Show scheduled appointments
0  - Exit
