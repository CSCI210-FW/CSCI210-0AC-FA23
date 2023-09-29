#include <iostream>
#include <sqlite3.h>
#include <string>
#include <ctime>
#include <limits>

void resetStream();
void codeGradeLoopFix(std::string errLocation);
int startTransaction(sqlite3 *);
int commit(sqlite3 *);
int rollback(sqlite3 *);
void makeSale(sqlite3 *);
int pickCustomer(sqlite3 *);
int makeInvoice(sqlite3 *, int);
void pickProduct(sqlite3 *, std::string &, int &, double &);
int makeline(sqlite3 *, int, std::string, int, double, int);
int updateProduct(sqlite3 *, std::string, int);
int updateCustomer(sqlite3 *, int, double);

int main()
{
    sqlite3 *db;
    int rc;

    rc = sqlite3_open_v2("SaleCo.db", &db, SQLITE_OPEN_READWRITE, NULL);
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
    makeSale(db);

    sqlite3_close(db);
    return 0;
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

/* make an invoice, which means we need a customer
   add lines to the invoice, which means we need products,
   update customer balance
   update the product qoh
*/
void makeSale(sqlite3 *db)
{
    int rc = startTransaction(db);
    if (rc != SQLITE_OK)
    {
        std::cout << "unable to start transaction. " << sqlite3_errmsg(db) << std::endl;
        return;
    }
    int cus_code = pickCustomer(db);
    if (cus_code == -1)
    {
        rollback(db);
        std::cout << "Error picking customer." << std::endl;
        return;
    }
    int invNum = makeInvoice(db, cus_code);
    if (invNum == -1)
    {
        rollback(db);
        std::cout << "Error making invoice." << std::endl;
        return;
    }
    char more = 'y';
    int lineNum = 1;
    double total = 0;
    do
    {
        std::string prod;
        int qoh;
        double price;
        pickProduct(db, prod, qoh, price);
        if (prod == "error")
        {
            rollback(db);
            std::cout << "Error picking product." << std::endl;
            return;
        }
        int rc = makeline(db, invNum, prod, qoh, price, lineNum);
        if (rc == -2)
            lineNum--;
        else if (rc == -1)
        {
            rollback(db);
            std::cout << "Error creating line." << std::endl;
            return;
        }
        else
        {
            total += rc * price;
            rc = updateProduct(db, prod, rc);
        }
        if (rc == -1)
        {
            rollback(db);
            std::cout << "Error updating product." << std::endl;
            return;
        }
        std::cout << "Would you like another product? ";
        std::cin >> more;
        std::cout << std::endl;
        lineNum++;

    } while (more == 'y');

    rc = updateCustomer(db, cus_code, total);
    if (rc == -1)
    {
        rollback(db);
        std::cout << "Error updating customer." << std::endl;
        return;
    }

    std::cout << "inserted invoice #" << invNum << std::endl;
    commit(db);
}

int pickCustomer(sqlite3 *db)
{
    std::string query = "select cus_code, cus_lname || ', ' || cus_fname as name ";
    query += "from customer";
    sqlite3_stmt *result;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &result, NULL);
    if (rc != SQLITE_OK)
    {
        sqlite3_finalize(result);
        std::cout << "There was an error with the customer query: " << sqlite3_errmsg(db) << std::endl;
        std::cout << query << std::endl;
        return -1;
    }
    int i, choice;
    std::cout << "Please choose a customer:" << std::endl;
    i = 1;
    for (rc = sqlite3_step(result); rc == SQLITE_ROW; rc = sqlite3_step(result))
    {
        std::cout << i << ". " << sqlite3_column_text(result, 0);
        std::cout << " - " << sqlite3_column_text(result, 1);
        std::cout << std::endl;
        i++;
    }
    std::cin >> choice;
    while (!std::cin || choice < 1 || choice >= i)
    {
        codeGradeLoopFix("get customer line 149");
        if (!std::cin)
            resetStream();
        std::cout << "That is not a valid choice! Try again!" << std::endl;
        std::cin >> choice;
    }
    sqlite3_reset(result);
    for (int i = 0; i < choice; i++)
        sqlite3_step(result);
    int cus_code = sqlite3_column_int(result, 0);
    sqlite3_finalize(result);
    return cus_code;
}

int makeInvoice(sqlite3 *db, int customer)
{
    char formatDate[80];
    time_t currentDate = time(NULL);
    strftime(formatDate, 80, "%F %T", localtime(&currentDate));
    std::string invDate(formatDate);

    std::string query = "insert into invoice (cus_code, inv_date) values (";
    query += std::to_string(customer) + ", '";
    query += invDate + "')";
    int rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "unable to insert invoice." << sqlite3_errmsg(db) << std::endl;
        std::cout << query << std::endl;
        return -1;
    }

    int invNum = sqlite3_last_insert_rowid(db);
    return invNum;
}

void pickProduct(sqlite3 *db, std::string &prodCode, int &qoh, double &price)
{
    std::string query = "select p_code, p_descript, p_qoh, p_price ";
    query += "from product";
    sqlite3_stmt *result;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &result, NULL);
    if (rc != SQLITE_OK)
    {
        sqlite3_finalize(result);
        std::cout << "There was an error with the product query: " << sqlite3_errmsg(db) << std::endl;
        std::cout << query << std::endl;
        prodCode = "error";
        return;
    }
    int i, choice;
    std::cout << "Please choose a product:" << std::endl;
    i = 1;
    for (rc = sqlite3_step(result); rc == SQLITE_ROW; rc = sqlite3_step(result))
    {
        std::cout << i << ". " << sqlite3_column_text(result, 0);
        std::cout << " - " << sqlite3_column_text(result, 1);
        std::cout << " (Quantity on Hand: " << sqlite3_column_text(result, 2) << ")";
        std::cout << std::endl;
        i++;
    }
    std::cin >> choice;
    while (!std::cin || choice < 1 || choice >= i)
    {
        codeGradeLoopFix("get product line 214");
        if (!std::cin)
            resetStream();
        std::cout << "That is not a valid choice! Try again!" << std::endl;
        std::cin >> choice;
    }
    sqlite3_reset(result);
    for (int i = 0; i < choice; i++)
        sqlite3_step(result);
    qoh = sqlite3_column_int(result, 2);
    price = sqlite3_column_double(result, 3);
    prodCode = reinterpret_cast<const char *>(sqlite3_column_text(result, 0));
    sqlite3_finalize(result);
}

int makeline(sqlite3 *db, int inv, std::string prod, int qoh, double price, int num)
{
    int quantity;
    std::cout << "How many would you like? (Quantity on hand: " << qoh << ") Enter 0 to cancel: ";
    std::cin >> quantity;
    std::cout << std::endl;

    while (!std::cin || quantity < 1 || quantity > qoh)
    {
        codeGradeLoopFix("line 256");
        if (!std::cin)
            resetStream();
        else if (quantity == 0)
        {
            return -2;
        }
        std::cout << "The amount entered is invalid. Please try again." << std::endl;
        std::cout << "How many would you like? (Quantity on hand: " << qoh << ") Enter 0 to cancel: ";
        std::cin >> quantity;
        std::cout << std::endl;
    }
    std::string query = "insert into line (inv_number, line_number, p_code, line_units, line_price) values (";
    query += std::to_string(inv) + ", ";
    query += std::to_string(num) + ", ";
    query += "'" + prod + "', ";
    query += std::to_string(quantity) + ", ";
    query += std::to_string(price) + ")";
    int rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "unable to insert line." << sqlite3_errmsg(db) << std::endl;
        std::cout << query << std::endl;
        return -1;
    }

    return quantity;
}

int updateProduct(sqlite3 *db, std::string prodCode, int q)
{
    std::string query = "update product set p_qoh = p_qoh - ";
    query += std::to_string(q);
    query += " where p_code = '" + prodCode + "'";
    int rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "unable to update product." << sqlite3_errmsg(db) << std::endl;
        std::cout << query << std::endl;
        return -1;
    }

    return 0;
}

int updateCustomer(sqlite3 *db, int cus, double amt)
{
    std::string query = "update customer set cus_balance = cus_balance + " + std::to_string(amt);
    query += " where cus_code = " + std::to_string(cus);
    int rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "unable to update customer." << sqlite3_errmsg(db) << std::endl;
        std::cout << query << std::endl;
        return -1;
    }

    return 0;
}