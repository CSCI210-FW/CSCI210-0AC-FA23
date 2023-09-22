#include <iostream>
#include <sqlite3.h>
#include <string>

int movieCallback(void *, int, char **, char **);
int actorCallback(void *, int, char **, char **);

int main()
{
    sqlite3 *db;
    int rc;

    rc = sqlite3_open_v2("IMDB.db", &db, SQLITE_OPEN_READWRITE, NULL);
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
    std::string query = "select distinct movie.name as 'Movie', movie.year as 'Year', movie.id ";
    query += "from movie ";
    query += "where movie.name like \"Shrek%\"";
    rc = sqlite3_exec(db, query.c_str(), movieCallback, db, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "There was an error - movie callback: " << sqlite3_errmsg(db) << std::endl;
        std::cout << query << std::endl;
    }

    sqlite3_close(db);
    return 0;
}

// select distinct movie.name as 'Movie', movie.year as 'Year', movie.id from movie wehre movie.name like 'Shrek$'
int movieCallback(void *extData, int numCols, char **values, char **colNames)
{
    for (int i = 0; i < numCols - 1; i++)
    {
        std::cout << colNames[i] << ": ";
        if (values[i] != NULL)
            std::cout << values[i];
        std::cout << std::endl;
    }
    std::cout << "Cast:" << std::endl;
    std::string query = "select actor.first_name, actor.last_name, c.role ";
    query += "from actor join cast as c on actor.id = c.actor_id ";
    query += "where c.movie_id = ";
    query += values[numCols - 1];
    sqlite3 *db = (sqlite3 *)extData;
    int rc = sqlite3_exec(db, query.c_str(), actorCallback, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "There was an error before actor callback: " << sqlite3_errmsg(db) << std::endl;
        std::cout << query << std::endl;
        return SQLITE_ERROR;
    }

    return SQLITE_OK;
}

int actorCallback(void *extData, int numCols, char **values, char **colNames)
{
    std::cout << "\t";
    if (values[0] != NULL)
        std::cout << values[0] << " ";
    if (values[1] != NULL)
        std::cout << values[1] << " ";
    std::cout << "- ";
    if (values[2] != NULL)
        std::cout << values[2];
    std::cout << std::endl;

    return SQLITE_OK;
}
