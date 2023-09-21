#include <iostream>
#include <sqlite3.h>
#include <string>

int main()
{
    sqlite3 *db;
    int rc;

    rc = sqlite3_open_v2("ConstructCo.db", &db, SQLITE_OPEN_READWRITE, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 0;
    }
    else
    {
        std::cout << "Database opened successfully." << std::endl;
    }

    sqlite3_close(db);
    return 0;
}