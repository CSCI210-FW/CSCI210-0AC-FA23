#include <iostream>
#include <sqlite3.h>
#include <string>

int callback(void *, int, char **, char **);

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
    std::string query = "select * from employee";
    rc = sqlite3_exec(db, query.c_str(), callback, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "There was an error - select callback: " << sqlite3_errmsg(db) << std::endl;
        std::cout << query << std::endl;
    }

    sqlite3_close(db);
    return 0;
}

int callback(void *extData, int numCols, char **values, char **colNames)
{
    for (int i = 0; i < numCols; i++)
    {
        std::cout << colNames[i] << ": ";
        if (values[i] != NULL)
            std::cout << values[i];
        std::cout << std::endl;
    }
    std::cout << std::endl;
    return SQLITE_OK;
}
