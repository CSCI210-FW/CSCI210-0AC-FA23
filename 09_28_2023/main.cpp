#include <iostream>
#include <sqlite3.h>
#include <string>
#include <ctime>
#include <limits>

int callback(void *, int, char **, char **);
void viewAssignmentsByProject(sqlite3 *);
void resetStream();
void codeGradeLoopFix(std::string errLocation);
int startTransaction(sqlite3 *);
int commit(sqlite3 *);
int rollback(sqlite3 *);

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
    query = "delete from employee where emp_lname = 'Brown' and emp_fname = 'Charlie'";
    rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "There was an error - delete: " << sqlite3_errmsg(db) << std::endl;
        std::cout << query << std::endl;
    }
    std::string lname, fname, mi;
    int job, years;
    char formatDate[80];
    time_t currentDate = time(NULL);
    strftime(formatDate, 80, "%F %T", localtime(&currentDate));
    std::string hiredate(formatDate);
    lname = "Brown";
    fname = "Charlie";
    job = 504;
    years = 0;

    query = "insert into employee(emp_lname, emp_fname, emp_hiredate, job_code, emp_years) ";
    query += "values ('";
    query += lname + "', '";
    query += fname + "', '";
    query += hiredate + "', ";
    query += std::to_string(job) + ", ";
    query += std::to_string(years) + ");";

    rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "There was an error - insert: " << sqlite3_errmsg(db) << std::endl;
        std::cout << query << std::endl;
    }
    else
    {
        int emp_num = sqlite3_last_insert_rowid(db);
        std::cout << fname << " " << lname << " inserted into the database as employee number " << emp_num << std::endl;
    }
    viewAssignmentsByProject(db);
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

void viewAssignmentsByProject(sqlite3 *db)
{
    std::string query;
    sqlite3_stmt *result;
    query = "select proj_num, proj_name from project";
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &result, NULL);
    if (rc != SQLITE_OK)
    {
        sqlite3_finalize(result);
        std::cout << "There was an error with the project query: " << sqlite3_errmsg(db) << std::endl;
        std::cout << query << std::endl;
        return;
    }
    int columnCount = sqlite3_column_count(result);
    int i, choice;
    std::cout << std::left;
    std::cout << "Please choose the project: " << std::endl;
    i = 1;
    do
    {
        rc = sqlite3_step(result);
        if (rc == SQLITE_DONE)
            break;
        std::cout << i << ". " << sqlite3_column_text(result, 0);
        std::cout << " - " << sqlite3_column_text(result, 1);
        std::cout << std::endl;
        i++;

    } while (rc == SQLITE_ROW);
    std::cin >> choice;
    while (!std::cin || choice < 1 || choice >= i)
    {
        codeGradeLoopFix("get project line 122");
        if (!std::cin)
            resetStream();
        std::cout << "That is not a valid choice! Try again!" << std::endl;
        std::cin >> choice;
    }
    sqlite3_reset(result);
    for (int j = 0; j < choice; j++)
    {
        sqlite3_step(result);
    }
    int proj_num = sqlite3_column_int(result, 0);
    sqlite3_finalize(result);
    query = "select sum(assign_hours) as 'Total Hours', sum(assign_charge) as 'Total Charges' ";
    query += "from assignment ";
    query += "where proj_num = " + std::to_string(proj_num);
    query += " group by proj_num";

    rc = sqlite3_prepare_v2(db, query.c_str(), -1, &result, NULL);
    if (rc != SQLITE_OK)
    {
        sqlite3_finalize(result);
        std::cout << "There was an error with the assignment query: " << sqlite3_errmsg(db) << std::endl;
        std::cout << query << std::endl;
        return;
    }
    columnCount = sqlite3_column_count(result);
    for (rc = sqlite3_step(result); rc == SQLITE_ROW; rc = sqlite3_step(result))
    {
        for (int i = 0; i < columnCount; i++)
        {
            std::cout << sqlite3_column_name(result, i) << " - ";
            if (sqlite3_column_type(result, i) != SQLITE_NULL)
            {
                std::cout << sqlite3_column_text(result, i);
            }
            std::cout << std::endl;
        }
    }
    sqlite3_finalize(result);
}

void codeGradeLoopFix(std::string errLocation)
{
    if (std::cin.eof())
    {
        std::cout << "There was a problem and there is no more input! @" + errLocation << std::endl;
        throw std::invalid_argument(errLocation); // return or throw an exception here to force the program to end or return from the function.
    }
}

void resetStream()
{
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int startTransaction(sqlite3 *db)
{
    std::string query = "begin transaction";
    int rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "There was an error - starting transaction: " << sqlite3_errmsg(db) << std::endl;
        return rc;
    }
    return SQLITE_OK;
}

int commit(sqlite3 *db)
{
    std::string query = "commit";
    int rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "There was an error - committing transaction: " << sqlite3_errmsg(db) << std::endl;
        rollback(db);
        return rc;
    }
    return SQLITE_OK;
}

int rollback(sqlite3 *db)
{
    std::string query = "rollback";
    int rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "There was an error - rolling back the transaction: " << sqlite3_errmsg(db) << std::endl;
        // rollback(db);
        return rc;
    }
    return SQLITE_OK;
}
