#include <iostream>
#include <string>
#include <sqlite3.h>
#include <limits>
using namespace std;

sqlite3* db=nullptr;

void execSQL(const string& sql){
    char* err=nullptr;
    if(sqlite3_exec(db,sql.c_str(),nullptr,nullptr,&err)!=SQLITE_OK){
        cerr<<"Database error: "<<(err?err:"unknown")<<"\n"; sqlite3_free(err);
    }
}
void initDB(){
    execSQL(R"SQL(
    PRAGMA foreign_keys=ON;
    CREATE TABLE IF NOT EXISTS users(
      id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, role TEXT NOT NULL
    );
    CREATE TABLE IF NOT EXISTS bins(
      id INTEGER PRIMARY KEY AUTOINCREMENT, location TEXT NOT NULL,
      bin_type TEXT NOT NULL CHECK(bin_type IN ('PLASTIC','PAPER','METAL','ORGANIC','E-WASTE','MIXED')),
      capacity REAL DEFAULT 100, current_fill REAL DEFAULT 0,
      status TEXT DEFAULT 'ACTIVE' CHECK(status IN ('ACTIVE','FULL','MAINTENANCE'))
    );
    CREATE TABLE IF NOT EXISTS collections(
      id INTEGER PRIMARY KEY AUTOINCREMENT, bin_id INTEGER NOT NULL,
      collector_id INTEGER NOT NULL, quantity REAL NOT NULL,
      collected_at TEXT DEFAULT CURRENT_TIMESTAMP,
      FOREIGN KEY(bin_id) REFERENCES bins(id), FOREIGN KEY(collector_id) REFERENCES users(id)
    );
    CREATE TABLE IF NOT EXISTS rewards(
      id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER NOT NULL,
      points INTEGER NOT NULL, reason TEXT NOT NULL, created_at TEXT DEFAULT CURRENT_TIMESTAMP,
      FOREIGN KEY(user_id) REFERENCES users(id)
    );
    )SQL");
}
void seed(){
    execSQL("INSERT INTO users(name,role) SELECT 'Student A','STUDENT' WHERE NOT EXISTS(SELECT 1 FROM users WHERE name='Student A');");
    execSQL("INSERT INTO users(name,role) SELECT 'Collector B','COLLECTOR' WHERE NOT EXISTS(SELECT 1 FROM users WHERE name='Collector B');");
    execSQL("INSERT INTO bins(location,bin_type,capacity,current_fill) SELECT 'Block A','PLASTIC',100,30 WHERE NOT EXISTS(SELECT 1 FROM bins WHERE location='Block A');");
    execSQL("INSERT INTO bins(location,bin_type,capacity,current_fill) SELECT 'Canteen','ORGANIC',100,75 WHERE NOT EXISTS(SELECT 1 FROM bins WHERE location='Canteen');");
    execSQL("INSERT INTO bins(location,bin_type,capacity,current_fill) SELECT 'Library','PAPER',100,45 WHERE NOT EXISTS(SELECT 1 FROM bins WHERE location='Library');");
}
int askInt(const string&m){int x;cout<<m;while(!(cin>>x)){cin.clear();cin.ignore(numeric_limits<streamsize>::max(),'\n');cout<<"Enter number: ";}cin.ignore(numeric_limits<streamsize>::max(),'\n');return x;}
double askDouble(const string&m){double x;cout<<m;while(!(cin>>x)){cin.clear();cin.ignore(numeric_limits<streamsize>::max(),'\n');cout<<"Enter number: ";}cin.ignore(numeric_limits<streamsize>::max(),'\n');return x;}
string ask(const string&m){string s;cout<<m;getline(cin,s);return s;}

void users(){
 sqlite3_stmt*st=nullptr; sqlite3_prepare_v2(db,"SELECT id,name,role FROM users ORDER BY id",-1,&st,nullptr);
 cout<<"\nID | Name | Role\n-------------------------\n";
 while(sqlite3_step(st)==SQLITE_ROW) cout<<sqlite3_column_int(st,0)<<" | "<<sqlite3_column_text(st,1)<<" | "<<sqlite3_column_text(st,2)<<"\n";
 sqlite3_finalize(st);
}
void bins(){
 sqlite3_stmt*st=nullptr; sqlite3_prepare_v2(db,"SELECT id,location,bin_type,capacity,current_fill,status,ROUND(current_fill*100.0/capacity,1) FROM bins ORDER BY id",-1,&st,nullptr);
 cout<<"\nID | Location | Type | Fill | Capacity | % | Status\n------------------------------------------------------\n";
 while(sqlite3_step(st)==SQLITE_ROW)
   cout<<sqlite3_column_int(st,0)<<" | "<<sqlite3_column_text(st,1)<<" | "<<sqlite3_column_text(st,2)
       <<" | "<<sqlite3_column_double(st,4)<<" | "<<sqlite3_column_double(st,3)<<" | "<<sqlite3_column_double(st,6)
       <<" | "<<sqlite3_column_text(st,5)<<"\n";
 sqlite3_finalize(st);
}
void addBin(){
 string loc=ask("Location: "); string type=ask("Type (PLASTIC/PAPER/METAL/ORGANIC/E-WASTE/MIXED): "); double cap=askDouble("Capacity (kg): ");
 sqlite3_stmt*st=nullptr; sqlite3_prepare_v2(db,"INSERT INTO bins(location,bin_type,capacity) VALUES(?,?,?)",-1,&st,nullptr);
 sqlite3_bind_text(st,1,loc.c_str(),-1,SQLITE_TRANSIENT);sqlite3_bind_text(st,2,type.c_str(),-1,SQLITE_TRANSIENT);sqlite3_bind_double(st,3,cap);
 sqlite3_step(st);sqlite3_finalize(st);cout<<"Bin added. ID "<<sqlite3_last_insert_rowid(db)<<"\n";
}
void addWaste(){
 int uid=askInt("Student user ID: "); int bid=askInt("Bin ID: "); double kg=askDouble("Waste quantity (kg): ");
 if(kg<=0){cout<<"Invalid quantity.\n";return;}
 sqlite3_stmt*st=nullptr; sqlite3_prepare_v2(db,"SELECT current_fill,capacity FROM bins WHERE id=?",-1,&st,nullptr);sqlite3_bind_int(st,1,bid);
 if(sqlite3_step(st)!=SQLITE_ROW){cout<<"Bin not found.\n";sqlite3_finalize(st);return;}
 double fill=sqlite3_column_double(st,0), cap=sqlite3_column_double(st,1);sqlite3_finalize(st);
 if(fill+kg>cap){cout<<"Bin capacity exceeded. Choose another bin or request collection.\n";return;}
 sqlite3_prepare_v2(db,"UPDATE bins SET current_fill=current_fill+?,status=CASE WHEN current_fill+?>=capacity THEN 'FULL' ELSE 'ACTIVE' END WHERE id=?",-1,&st,nullptr);
 sqlite3_bind_double(st,1,kg);sqlite3_bind_double(st,2,kg);sqlite3_bind_int(st,3,bid);sqlite3_step(st);sqlite3_finalize(st);
 int points=(int)(kg*10);
 sqlite3_prepare_v2(db,"INSERT INTO rewards(user_id,points,reason) VALUES(?,?,?)",-1,&st,nullptr);
 string reason="Correct waste disposal";sqlite3_bind_int(st,1,uid);sqlite3_bind_int(st,2,points);sqlite3_bind_text(st,3,reason.c_str(),-1,SQLITE_TRANSIENT);sqlite3_step(st);sqlite3_finalize(st);
 cout<<"Waste recorded. Reward points: "<<points<<"\n";
}
void collect(){
 int bid=askInt("Bin ID: ");int cid=askInt("Collector user ID: ");
 sqlite3_stmt*st=nullptr;sqlite3_prepare_v2(db,"SELECT current_fill FROM bins WHERE id=?",-1,&st,nullptr);sqlite3_bind_int(st,1,bid);
 if(sqlite3_step(st)!=SQLITE_ROW){cout<<"Bin not found.\n";sqlite3_finalize(st);return;}double qty=sqlite3_column_double(st,0);sqlite3_finalize(st);
 if(qty<=0){cout<<"Bin is empty.\n";return;}
 sqlite3_prepare_v2(db,"INSERT INTO collections(bin_id,collector_id,quantity) VALUES(?,?,?)",-1,&st,nullptr);
 sqlite3_bind_int(st,1,bid);sqlite3_bind_int(st,2,cid);sqlite3_bind_double(st,3,qty);sqlite3_step(st);sqlite3_finalize(st);
 sqlite3_prepare_v2(db,"UPDATE bins SET current_fill=0,status='ACTIVE' WHERE id=?",-1,&st,nullptr);sqlite3_bind_int(st,1,bid);sqlite3_step(st);sqlite3_finalize(st);
 cout<<"Collection recorded: "<<qty<<" kg removed.\n";
}
void rewards(){
 int uid=askInt("User ID: ");sqlite3_stmt*st=nullptr;
 sqlite3_prepare_v2(db,"SELECT COALESCE(SUM(points),0) FROM rewards WHERE user_id=?",-1,&st,nullptr);sqlite3_bind_int(st,1,uid);
 if(sqlite3_step(st)==SQLITE_ROW) cout<<"Total reward points: "<<sqlite3_column_int(st,0)<<"\n";sqlite3_finalize(st);
 sqlite3_prepare_v2(db,"SELECT reason,points,created_at FROM rewards WHERE user_id=? ORDER BY id DESC",-1,&st,nullptr);sqlite3_bind_int(st,1,uid);
 while(sqlite3_step(st)==SQLITE_ROW) cout<<sqlite3_column_text(st,2)<<" | +"<<sqlite3_column_int(st,1)<<" | "<<sqlite3_column_text(st,0)<<"\n";
 sqlite3_finalize(st);
}
void report(){
 sqlite3_stmt*st=nullptr;
 sqlite3_prepare_v2(db,"SELECT COUNT(*),COALESCE(SUM(quantity),0) FROM collections",-1,&st,nullptr);
 if(sqlite3_step(st)==SQLITE_ROW) cout<<"\nCollections completed: "<<sqlite3_column_int(st,0)<<" | Total waste collected: "<<sqlite3_column_double(st,1)<<" kg\n";
 sqlite3_finalize(st);
 cout<<"Bins at/above 80% capacity:\n";
 sqlite3_prepare_v2(db,"SELECT location,bin_type,ROUND(current_fill*100.0/capacity,1) FROM bins WHERE current_fill*100.0/capacity>=80 ORDER BY current_fill*1.0/capacity DESC",-1,&st,nullptr);
 while(sqlite3_step(st)==SQLITE_ROW) cout<<"- "<<sqlite3_column_text(st,0)<<" ("<<sqlite3_column_text(st,1)<<") : "<<sqlite3_column_double(st,2)<<"%\n";
 sqlite3_finalize(st);
}
int main(){
 if(sqlite3_open("ecotrack.db",&db)!=SQLITE_OK){cerr<<"Cannot open database\n";return 1;}initDB();seed();
 int ch;do{
  cout<<"\n========== ECOTRACK ==========\n1. View users\n2. View bins\n3. Add bin\n4. Record waste disposal\n5. Collect/empty bin\n6. View reward points\n7. Sustainability report\n0. Exit\n";
  ch=askInt("Choose: ");
  switch(ch){case 1:users();break;case 2:bins();break;case 3:addBin();break;case 4:addWaste();break;case 5:collect();break;case 6:rewards();break;case 7:report();break;case 0:break;default:cout<<"Invalid option.\n";}
 }while(ch!=0);
 sqlite3_close(db);return 0;
}
