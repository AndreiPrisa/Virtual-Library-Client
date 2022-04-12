#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

// macro-uri pentru server ip si port
#define IP_SERVER "34.118.48.238"
#define PORT 8080

// variabila globala pentru a retine cookie-ul de login al utilizatorului curent
char *login_cookie = NULL;

// JWT Token pentru a demonstra accesul la library
char *JWT_token = NULL;

// functie ajutatoare care extrage din mesajul dat de catre server eroarea si o afiseaza
void print_error(char *response) {
    char *json_string = strstr(response, "{");
    JSON_Value *data = json_parse_string(json_string);
    char *msg = strdup(json_object_get_string(json_object(data), "error"));
    printf("[Error] %s\n", msg);
    json_value_free(data);
}


// functie pentru a inregistra un nou utilizator
void register_user(int sockfd) {
    char username[BUFLEN], password[BUFLEN];
    char *message, *response;
    char **body = malloc(sizeof(char*));

    printf("username=");
    memset(username, 0, BUFLEN);
    fgets(username, BUFLEN - 1, stdin);
    if (username[strlen(username) - 1] == '\n')
        username[strlen(username) - 1] = 0;

    memset(password, 0, BUFLEN);
    printf("password=");
    fgets(password, BUFLEN - 1, stdin);
    if (password[strlen(password) - 1] == '\n')
        password[strlen(password) - 1] = 0;

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);

    char *string = NULL;
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    string = json_serialize_to_string_pretty(root_value);
    body[0] = strdup(string);

    json_free_serialized_string(string);
    json_value_free(root_value);

    message = compute_post_request(IP_SERVER, "/api/v1/tema/auth/register", "application/json", body, 1, NULL, 0, NULL);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    // afisam mesaj corespunzator
    if (strstr(response, "error") != NULL) {
        print_error(response);
    }
    else {
        puts("[SUCCESS] User registered successfully!");
    }

    close_connection(sockfd);
}

// functie pentru a loga un utilizator
void login_user(int sockfd) {
    char username[BUFLEN], password[BUFLEN];
    char *message, *response;
    char **body = malloc(sizeof(char*));

    printf("username=");
    memset(username, 0, BUFLEN);
    fgets(username, BUFLEN - 1, stdin);
    if (username[strlen(username) - 1] == '\n')
        username[strlen(username) - 1] = 0;

    memset(password, 0, BUFLEN);
    printf("password=");
    fgets(password, BUFLEN - 1, stdin);
    if (password[strlen(password) - 1] == '\n')
        password[strlen(password) - 1] = 0;

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);

    char *string = NULL;
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    string = json_serialize_to_string_pretty(root_value);
    body[0] = strdup(string);
    json_free_serialized_string(string);
    json_value_free(root_value);

    message = compute_post_request(IP_SERVER, "/api/v1/tema/auth/login", "application/json", body, 1, NULL, 0, NULL);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    // afisam mesaj corespunzator
    if (strstr(response, "error") != NULL) {
        print_error(response);
    }
    else {
        puts("[SUCCESS] User connected successfully!");
        char *begin = strstr(response, "connect.sid=");
        login_cookie = strdup(strtok(begin, " "));

    }

    close_connection(sockfd);
}

// functie pentru a deconecta utilizatorul curent
void logout_user(int sockfd) {
    char *message, *response;
    char **cookies = malloc(sizeof(char*));

    if (login_cookie != NULL) {

        cookies[0] = login_cookie;
        message = compute_get_request(IP_SERVER, "/api/v1/tema/auth/logout", NULL, cookies, 1, JWT_token);
    }
    else
        message = compute_get_request(IP_SERVER, "/api/v1/tema/auth/logout", NULL, NULL, 0, JWT_token);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    // afisam mesaj corespunzator
    if (strstr(response, "error") != NULL) {
        print_error(response);
    }
    else {
        puts("[SUCCESS] Logout successful!");

        // resetam cookie-ul de login si JWT Token
        JWT_token = NULL;
        login_cookie = NULL;
    }

    close_connection(sockfd);
}

// functie pentru a intra in library
void enter_library(int sockfd) {
    char **cookies = malloc(sizeof(char*));
    char *message, *response;

    if (login_cookie != NULL) {
        cookies[0] = login_cookie;
        message = compute_get_request(IP_SERVER, "/api/v1/tema/library/access", NULL, cookies, 1, NULL);
    } else {
        message = compute_get_request(IP_SERVER, "/api/v1/tema/library/access", NULL, NULL, 0, NULL);
    }
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    // afisam mesaj corespunzator
    if (strstr(response, "error") != NULL) {
        print_error(response);
    }
    else {
        puts("[SUCCESS] Token received!");
        char *json_string = strstr(response, "{");

        JSON_Value *data = json_parse_string(json_string);
        JWT_token = strdup(json_object_get_string(json_object(data), "token"));
        json_value_free(data);
    }

    close_connection(sockfd);
}

// functie pentru a extrage informatii despre o carte din library dupa id-ul sau
void get_book(int sockfd) {
    char id[BUFLEN];
    char *message, *response;

    printf("id=");
    memset(id, 0, BUFLEN);
    fgets(id, BUFLEN - 1, stdin);
    if (id[strlen(id) - 1] == '\n')
        id[strlen(id) - 1] = 0;
    
    if (strlen(id) == 0) {
        puts("[ERROR] ID is missing!");
        close_connection(sockfd);
        return;
    }
    char access[BUFLEN];
    memset(access, 0, BUFLEN);

    strcpy(access, "/api/v1/tema/library/books/");
    strcat(access, id);

    message = compute_get_request(IP_SERVER, access, NULL, NULL, 0, JWT_token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    // afisam mesaj corespunzator
    if (strstr(response, "error") != NULL) {
        print_error(response);
    }
    else {
        puts("[SUCCESS] Book found!");
        char *json_string = strstr(response, "{");
        JSON_Value *data = json_parse_string(json_string);

        // afisam detaliile cartii pe care am cautat-o
        printf("========\nTitle: %s\nAuthor: %s\nPublisher: %s\nGenre: %s\nPage count: %.0f\n========\n"
         , json_object_get_string(json_object(data), "title"), json_object_get_string(json_object(data), "author"),
         json_object_get_string(json_object(data), "publisher"), json_object_get_string(json_object(data), "genre"),
         json_object_get_number(json_object(data), "page_count"));
        json_value_free(data);
    }

    close_connection(sockfd);
}

// functie care afiseaza cartile din library (doar id-ul si titlul)
void get_books(int sockfd) {
    char *message, *response;
    message = compute_get_request(IP_SERVER, "/api/v1/tema/library/books", NULL, NULL, 0, JWT_token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    // afisam mesaj corespunzator
    if (strstr(response, "error") != NULL) {
        puts("[ERROR] You do not have access to get books!");
    }
    else {
        puts("[SUCCESS] Access granted!");
        int i = 0;
        char *json_string = strstr(response, "[");
        JSON_Value *data = json_parse_string(json_string);
        JSON_Array *array = json_value_get_array(data);
        JSON_Object *element;

        // afisam id-ul si titlul cartilor 
        puts("========");
        for (i = 0; i < json_array_get_count(array); i++) {
            element = json_array_get_object(array, i);
            printf("ID: %d, Title: %s\n", (int) json_object_get_number(element, "id"), json_object_get_string(element, "title")); 
        }

        // afisam mesaj corespunzator daca nu avem nicio carte
        if (i == 0) {
            puts("Bookshelf empty!");
        }
        puts("========");

        json_value_free(data);

    }

    close_connection(sockfd);
}

// functie pentru a adauga o noua carte in library
void add_book(int sockfd) {
    char title[BUFLEN], author[BUFLEN], genre[BUFLEN], publisher[BUFLEN], page_count[BUFLEN];
    char *message, *response;
    char **body = malloc(sizeof(char*));

    printf("title=");
    memset(title, 0, BUFLEN);
    fgets(title, BUFLEN - 1, stdin);
    if (title[strlen(title) - 1] == '\n')
        title[strlen(title) - 1] = 0;

    memset(author, 0, BUFLEN);
    printf("author=");
    fgets(author, BUFLEN - 1, stdin);
    if (author[strlen(author) - 1] == '\n')
        author[strlen(author) - 1] = 0;

    memset(genre, 0, BUFLEN);
    printf("genre=");
    fgets(genre, BUFLEN - 1, stdin);
    if (genre[strlen(genre) - 1] == '\n')
        genre[strlen(genre) - 1] = 0;

    memset(publisher, 0, BUFLEN);
    printf("publisher=");
    fgets(publisher, BUFLEN - 1, stdin);
    if (publisher[strlen(publisher) - 1] == '\n')
        publisher[strlen(publisher) - 1] = 0;

    memset(page_count, 0, BUFLEN);
    printf("page_count=");
    fgets(page_count, BUFLEN - 1, stdin);
    if (page_count[strlen(page_count) - 1] == '\n')
        page_count[strlen(page_count) - 1] = 0;

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);

    char *string = NULL;

    // setam campul pe 0 in cazul in care noi nu completam nimic (cu scopul de a produce)
    if (strlen(title) == 0)
        json_object_set_string(root_object, "title", 0);
    else
        json_object_set_string(root_object, "title", title);

    if (strlen(author) == 0)
        json_object_set_string(root_object, "author", 0);
    else
        json_object_set_string(root_object, "author", author);

    if (strlen(genre) == 0)
        json_object_set_string(root_object, "genre", 0);
    else
        json_object_set_string(root_object, "genre", genre);

    if (strlen(publisher) == 0)
        json_object_set_string(root_object, "publisher", 0);
    else
        json_object_set_string(root_object, "publisher", publisher);
    
    if (strlen(page_count) == 0)
        json_object_set_string(root_object, "page_count", 0);
    else
        json_object_set_number(root_object, "page_count", atoi(page_count));

    

    string = json_serialize_to_string_pretty(root_value);
    body[0] = strdup(string);
    json_free_serialized_string(string);
    json_value_free(root_value);

    message = compute_post_request(IP_SERVER, "/api/v1/tema/library/books", "application/json", body, 1, NULL, 0, JWT_token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    // afisam mesaj corespunzator
    if (strstr(response, "error") != NULL) {
        print_error(response);
    }
    else {
        puts("[SUCCESS] Book added successfully!");

    }

    close_connection(sockfd);
}

// functie pentru a sterge o carte din library dupa id
void delete_book(int sockfd) {
    char id[BUFLEN];
    char *message, *response;

    printf("id=");
    memset(id, 0, BUFLEN);
    fgets(id, BUFLEN - 1, stdin);
    if (id[strlen(id) - 1] == '\n')
        id[strlen(id) - 1] = 0;

    if (strlen(id) == 0) {
        puts("[ERROR] ID is missing!");
        close_connection(sockfd);
        return;
    }
    
    char access[BUFLEN];
    memset(access, 0, BUFLEN);

    strcpy(access, "/api/v1/tema/library/books/");
    strcat(access, id);

    message = compute_delete_request(IP_SERVER, access, NULL, NULL, 0, JWT_token);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    // afisam mesaj corespunzator
    if (strstr(response, "error") != NULL) {
        print_error(response);
    }
    else {
        puts("[SUCCESS] Book deleted successfully!");
    }

    close_connection(sockfd);
}

int main(int argc, char *argv[])
{
    int sockfd;
    char buffer[BUFLEN];

    while(1) {
        memset(buffer, 0, BUFLEN);
		fgets(buffer, BUFLEN - 1, stdin);

        // server inchis, non functional
        sockfd = open_connection(IP_SERVER, PORT, AF_INET, SOCK_STREAM, 0);

        // vedem ce comanda am introdus

        if (strlen(buffer) == 5 && strncmp(buffer, "exit", 4) == 0) {
            break;
        }

        if (strlen(buffer) == 9 && strncmp(buffer, "register", 8) == 0) {
            if (login_cookie != NULL) {
                puts("[ERROR] You are already logged in!");
                continue;
            }
            register_user(sockfd);
            continue;
        }
        if (strlen(buffer) == 6 && strncmp(buffer, "login", 5) == 0) {
            if (login_cookie != NULL) {
                puts("[ERROR] You are already logged in!");
                continue;
            }
            login_user(sockfd);
            continue;
        }
        if (strlen(buffer) == 14 && strncmp(buffer, "enter_library", 13) == 0) {
            enter_library(sockfd);
            continue;
        }
        if (strlen(buffer) == 10 && strncmp(buffer, "get_books", 9) == 0) {
            get_books(sockfd);
            continue;
        }
        if (strlen(buffer) == 9 && strncmp(buffer, "add_book", 8) == 0) {
            add_book(sockfd);
            continue;
        }

        if (strlen(buffer) == 9 && strncmp(buffer, "get_book", 8) == 0) {
            get_book(sockfd);
            continue;
        }

        if (strlen(buffer) == 7 && strncmp(buffer, "logout", 6) == 0) {
            logout_user(sockfd);
            continue;
        }

        if (strlen(buffer) == 12 && strncmp(buffer, "delete_book", 11) == 0) {
            delete_book(sockfd);
            continue;
        }

        close_connection(sockfd);
        puts("[ERROR] Invalid command!");
    }
    close_connection(sockfd);
    return 0;
}
