/*
 * Copyright (c) 2026 Bruce Botes (https://github.com/brucebotes/tskbible-pub)
 * All rights reserved.
 *
 * This source code is licensed under the MIT License found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <stdlib.h>
#include <asv-common.h>
#include <asv-sv.h>
#include "qs_parse.h"

#define NUMKVPAIRS 256
#define VALSIZE 256
constexpr size_t QUERY_STRING_MAX = 256;

extern void pretty_print_html_references(StringView ref);
extern void html_printf(const char *format, ...);

// forward declarations
void display_reference_form(const char *);
void display_book_names(void);
void display_alternative(void);

void display_page(const char *route) {
  char *kvpairs[NUMKVPAIRS];
  char * value_ptr;
  char query_reference_buf[QUERY_STRING_MAX] = {"null"};
  StringView query_reference = (StringView){.data=query_reference_buf, .len=strlen(query_reference_buf)};

  char *remote_host = getenv("REMOTE_HOST");
  char *query = getenv("QUERY_STRING");
  char *referer = getenv("HTTP_REFERER");
  if (remote_host == NULL) {
    remote_host = "";
  }
  if (referer == NULL) {
    referer = "";
  }

  StringView reference = SV("null");
  if (query != NULL) {
    /*  ***THIS WILL ALTER query***  */
    int i = qs_parse(query, kvpairs, 256);
    /*  At this point qs_scanvalue() will no longer work with this query string */
    if ((value_ptr = qs_k2v("reference", kvpairs, i)) != NULL) {
      size_t ll = strlen(value_ptr);
      if( ll > 0 && ll < QUERY_STRING_MAX) {
	reference = (StringView){.data = value_ptr, .len = ll};
	// make a copy but replace all ' ' with '+' -> used in another query string as a source reference
	for (size_t i = 0; i < ll; ++i) {
	  if (reference.data[i] == ' ') {
	    query_reference_buf[i] = '+';
	  } else {
	    query_reference_buf[i] = reference.data[i];
	  }
	}
	query_reference.len = ll;
      }
    }
  }

  // RETURN STATUS - must have two CRLF's
  html_printf("Content-Type: text/html\r\n\r\n");

  //Start of document
  html_printf("<!DOCTYPE html>\n"
	      "<html>\n"
	      "<head>\n"
	      "<title>Reference Form</title>\n"
	      "<link rel=\"stylesheet\" href=\"/erbs/style.css\" type=\"text/css\"/>\n"
	      "<style>\n");
  html_printf("body:hover {\n"
	      "  border-image: url(\"/api/blog/tsk-%.*s/?title=easy-reader-tsk&ref=%s\");\n"
	      "}\n", (int)query_reference.len, query_reference.data, remote_host);
  html_printf(" .container {\n"
	      "   display: flex;\n"
	      "   margin-bottom: 1rem;\n"
	      "   margin-left: 0;\n"
	      "   margin-right: 1rem;\n"
	      " }\n"
	      " .left_child {\n"
	      "   margin: 0 1rem 0 1rem;\n"
	      " }\n"
	      " .right_child {\n"
	      "   margin: 0 5rem 0 1rem;\n"
	      " }\n"
	      " .tsk_container {\n"
	      "   margin: 1rem 5rem 1rem 0;"
	      " }\n"

   
	      "</style>\n"
	      "</head>\n"
	      "<body>\n");
  html_printf(  "<header id=\"preamble\" class=\"status\">\n"
		"<nav>\n"
		"  <a href=\"%s\">&lt;&lt;Back </a>\n"
		"  <a href=\"/blog/\">DELICIUM</a>\n"
		"  <a href=\"/\">Home</a>\n"
		"  <a href=\"/erbs/index.html\">Bullinger Structures</a>\n"
		"  <a href=\"/erbs/ps/index.html\">Psalm Structures</a>\n"
		"</nav>\n", referer);
  html_printf("<section>\n"
	      "<h1>Easy-Read Treasury of Scripture Knowldege</h1>\n"
  
	      "<div class=\"container\">\n"
	      "<div class=\"left_child\">\n"
	      "<p>Enter a bible reference of your choice:</p>\n"
	      "<p>examples: </p>\n"
	      "<pre>ps 1:1-5;joh 1:1</pre>\n"
	      " <p>or simply</p>\n"
	      "<pre>heb 1:1</pre>\n");  
  display_reference_form(route);
  html_printf("</div>\n");
  display_book_names();
  html_printf("</div>\n"
	      "<div class=\"tsk_container\">\n");

  
  if (sv_equals(reference, SV("null"))) {
    display_alternative();
  }else {
    html_printf("<p>Your scripture request is: %s</p>\n", reference);
    pretty_print_html_references(reference);
  }

  html_printf("</div>"
	      "</section>\n"
	      "</body></html>\n");
}

void display_alternative(void) {
      html_printf("<p>You did not provide a reference!</p>\n"
		  "<p>Displaying joh 1:1 as example!</p>\n");
      pretty_print_html_references(SV("joh 1:1"));
}

void display_reference_form(const char *route)
{
  html_printf("<form method=\"GET\" action=\"/%s\">\n", route);
  html_printf("<label for=\"reference\">Reference:</label><br>\n"
	      "<input type=\"text\" "
	      "id=\"reference\" "
	      "name=\"reference\" "
	      "required>\n"
	      "<br><br>\n"
	      "<input type=\"submit\" value=\"Submit\">\n"
	      "</form>\n");
}

void display_book_names(void){
  html_printf("<div class=\"right_child\">\n"
	      "<p>Book Name Abbreviations:</p>\n"
	      "<p>ge, ex, le, nu, de, jos, jud, ru, 1sa, 2sa, 1ki, 2ki, 1ch, 2ch, ezr, ne, es, job, ps, pr, ec, so, isa, jer, la, eze, da, ho, joe, am, ob, jon, mic, na, hab, zep, hag, zec, mal, mt, mr, lu, joh, ac, ro, 1co, 2co, ga, eph, php, col, 1th, 2th, 1ti, 2ti, tit, phm, heb, jas, 1pe, 2pe, 1jo, 2jo, 3jo, jude, re  </p>"
	      "</div>\n");
}

