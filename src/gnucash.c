/* ************************************************************************** */
/* Ce fichier s'occupe de la gestion du format gnucash			      */
/*                                                                            */
/*     Copyright (C)	2004      Benjamin Drieu (bdrieu@april.org)	      */
/* 			http://www.grisbi.org				      */
/*                                                                            */
/*  This program is free software; you can redistribute it and/or modify      */
/*  it under the terms of the GNU General Public License as published by      */
/*  the Free Software Foundation; either version 2 of the License, or         */
/*  (at your option) any later version.                                       */
/*                                                                            */
/*  This program is distributed in the hope that it will be useful,           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/*  GNU General Public License for more details.                              */
/*                                                                            */
/*  You should have received a copy of the GNU General Public License         */
/*  along with this program; if not, write to the Free Software               */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/*                                                                            */
/* ************************************************************************** */


#include "include.h"


/*START_INCLUDE*/
#include "gnucash.h"
#include "dialog.h"
#include "utils_str.h"
#include "utils_files.h"
#include "utils_xml.h"
/*END_INCLUDE*/

/*START_STATIC*/
static struct struct_compte_importation * find_imported_account_by_uid ( gchar * guid );
static struct gnucash_category * find_imported_categ_by_uid ( gchar * guid );
static gchar * get_currency ( xmlNodePtr currency_node );
static gdouble gnucash_value ( gchar * value );
static xmlDocPtr parse_gnucash_file ( gchar * filename );
static void recuperation_donnees_gnucash_book ( xmlNodePtr book_node );
static void recuperation_donnees_gnucash_categorie ( xmlNodePtr categ_node );
static void recuperation_donnees_gnucash_compte ( xmlNodePtr compte_node );
static void recuperation_donnees_gnucash_transaction ( xmlNodePtr transaction_node );
struct gnucash_split * find_split ( GSList * split_list, gdouble amount, 
				    struct struct_compte_importation * account, 
				    struct gnucash_category * categ );
void update_split ( struct gnucash_split * split, gdouble amount, gchar * account, 
		    gchar * categ );
struct gnucash_split * new_split ( gdouble amount, gchar * account, gchar * categ );
struct struct_compte_importation * find_imported_account_by_uid ( gchar * guid );
struct struct_compte_importation * find_imported_account_by_name ( gchar * name );
struct struct_ope_importation * new_transaction_from_split ( struct gnucash_split * split,
							     gchar * tiers, GDate * date );
/*END_STATIC*/

/*START_EXTERN*/
extern GSList *liste_comptes_importes;
extern gchar * tempname;
/*END_EXTERN*/

/* Structures */
struct gnucash_category {
  gchar * name;
  enum gnucash_category_type {
    GNUCASH_CATEGORY_INCOME,
    GNUCASH_CATEGORY_EXPENSE,
  } type;
  gchar * guid;
};

struct gnucash_split {
  gdouble amount;
  gchar * category;
  gchar * account;
  gchar * contra_account;
  gchar * notes;
  enum operation_etat_rapprochement p_r;
};

/* Variables */
GSList * gnucash_categories = NULL;



/**
 * Parse specified file as a Gnucash file and construct necessary data
 * structures with result.  Use a custom XML parser.
 *
 * \param filename	File to parse.
 *
 * \return TRUE upon success.  FALSE otherwise.
 */
gboolean recuperation_donnees_gnucash ( gchar * filename )
{
  xmlDocPtr doc;

  doc = parse_gnucash_file ( filename );

  if ( doc )
  {
      xmlNodePtr root = xmlDocGetRootElement(doc);
      xmlNodePtr root_node = NULL;

      if ( root )
      {
	  root_node = root -> children;
	  recuperation_donnees_gnucash_book ( root_node );
      }
      else
      {
	  return FALSE;
      }
  }
  
  return ( TRUE );
}



/**
 * Parse XML book nodes from a gnucash file.
 *
 * Main role of this function is to iterate over XML nodes of the file
 * and determine which account nodes are category nodes and which are
 * real accounts, as in Gnucash, accounts and categories are mixed.
 * 
 * \param book_node	Pointer to current XML node.
 */
void recuperation_donnees_gnucash_book ( xmlNodePtr book_node )
{
    xmlNodePtr child_node;
    
    child_node = book_node -> children;
    
    while ( child_node )
    {
	/* Books are subdivisions of gnucash files */
	if ( node_strcmp(root_node, "book") )
        {
	    recuperation_donnees_gnucash_book ( root_node );
	}

	if ( node_strcmp(child_node, "account") )
        {
	    gchar * type = child_content ( child_node, "type");
	    if ( strcmp(type, "INCOME") && strcmp(type, "EXPENSE") && strcmp(type, "EXPENSES") &&
	       strcmp(type, "EQUITY") )
	    {
		recuperation_donnees_gnucash_compte ( child_node );
	    }
	    else
		{
		    recuperation_donnees_gnucash_categorie ( child_node );
		}
	}
	
	if ( node_strcmp(child_node, "transaction") )
        {
	    recuperation_donnees_gnucash_transaction ( child_node );
	}
	
	child_node = child_node -> next;
    }
}



/**
 * Parse XML account node and fill a struct_compte_importation with
 * results.  Add account to the global accounts list
 * liste_comptes_importes.
 *
 * \param compte_node	XML account node to parse.
 */
void recuperation_donnees_gnucash_compte ( xmlNodePtr compte_node )
{
  struct struct_compte_importation *compte;
  gchar * type = child_content ( compte_node, "type" );

  compte = calloc ( 1, sizeof ( struct struct_compte_importation ));

  /* Gnucash import */
  compte -> origine = GNUCASH_IMPORT;

  if ( !strcmp(type, "BANK") || !strcmp(type, "CREDIT") ) 
    {
      compte -> type_de_compte = 0; /* Bank */
    }
  else if ( !strcmp(type, "CASH") || !strcmp(type, "CURRENCY") ) 
    {
      compte -> type_de_compte = 1; /* Currency */
    }
  else if ( !strcmp(type, "ASSET") || !strcmp(type, "STOCK") || !strcmp(type, "MUTUAL") ) 
    {
      compte -> type_de_compte = 0; /* Asset */
    }
  else if ( !strcmp(type, "LIABILITY") ) 
    {
      compte -> type_de_compte = 0; /* Liability */
    }

  compte -> nom_de_compte = child_content ( compte_node, "name" );
  compte -> solde = 0;
  compte -> devise = get_currency ( get_child(compte_node, "commodity") );
  compte -> guid = child_content ( compte_node, "id" );
  compte -> operations_importees = NULL;

  liste_comptes_importes = g_slist_append ( liste_comptes_importes, compte );
}



/**
 * Parse XML category node and fill a gnucash_category with results.
 * Add category to the global category list.
 *
 * \param categ_node	XML category node to parse.
 */
void recuperation_donnees_gnucash_categorie ( xmlNodePtr categ_node )
{
    struct gnucash_category * categ;

    categ = calloc ( 1, sizeof ( struct gnucash_category ));

    /* Find name, could be tricky if there is a parent. */
    categ -> name = child_content ( categ_node, "name" );
    if ( child_content ( categ_node, "parent" ) )
    {
	gchar * parent_guid = child_content ( categ_node, "parent" );
	GSList * liste_tmp = gnucash_categories;

	while ( liste_tmp )
	{
	    struct gnucash_category * iter = liste_tmp -> data;

	    if ( !strcmp ( iter -> guid, parent_guid ) )
	    {
		categ -> name = g_strconcat ( iter -> name, " : ", categ -> name, NULL );
		break;
	    }

	    liste_tmp = liste_tmp -> next;
	}
    }

    categ -> guid = child_content ( categ_node, "id" );

    /* Find if this is an expense or income category. */
    if ( !strcmp ( child_content ( categ_node, "type" ), "INCOME" ) )
    {
	categ -> type = GNUCASH_CATEGORY_INCOME;
    }
    else 
    {
	categ -> type = GNUCASH_CATEGORY_EXPENSE;
    }

    gnucash_categories = g_slist_append ( gnucash_categories, categ );
}



/**
 * Parse XML transaction node and fill a struct_ope_importation with results.
 *
 * \param transaction_node	XML transaction node to parse.
 */
void recuperation_donnees_gnucash_transaction ( xmlNodePtr transaction_node )
{
  struct struct_ope_importation * transaction;
  struct struct_compte_importation * account = NULL; 
  struct gnucash_split * split;
  gchar * date_string, *space, *tiers;
  GDate * date;
  xmlNodePtr splits, split_node, date_node;
  GSList * split_list = NULL;
  gdouble total = 0;

  /* Transaction amount, category, account, etc.. */
  splits = get_child ( transaction_node, "splits" );
  split_node = splits -> children;

  while ( split_node )
    {
      struct struct_compte_importation * split_account = NULL; 
      struct gnucash_category * categ = NULL;
      struct gnucash_split * split;
      enum operation_etat_rapprochement p_r = OPERATION_NORMALE;
      gdouble amount;

      /**
       * Gnucash transactions are in fact "splits", much like grisbi's
       * breakdowns of transactions.  We need to parse all splits and
       * see whether they are transfers to real accounts or transfers
       * to category accounts.  In that case, we only create one
       * transactions.  The other is discarded as grisbi is not a
       * double part financial engine.
       */
      if ( node_strcmp ( split_node, "split" ) )
	{
	  gchar * account_name = NULL, * categ_name = NULL;

	  split_account = find_imported_account_by_uid ( child_content ( split_node, 
									"account" ) );
	  categ = find_imported_categ_by_uid ( child_content ( split_node, "account" ) );
	  amount = gnucash_value ( child_content(split_node, "value") );

	  if ( categ ) 
	    categ_name = categ -> name;
	  if ( split_account )
	    {
	      /* All of this stuff is here since we are dealing with
		 the account split, not the category one */
	      account_name = split_account -> nom_de_compte;
	      total += amount;
	      if ( strcmp(child_content(split_node, "reconciled-state"), "n") )
		p_r = OPERATION_RAPPROCHEE;
	    }

	  split = find_split ( split_list, amount, split_account, categ );
	  if ( split )
	    {
	      update_split ( split, amount, account_name, categ_name );
	    }
	  else
	    {
	      split = new_split ( amount, account_name, categ_name );
	      split_list = g_slist_append ( split_list, split );
	      split -> notes = child_content(split_node, "memo");
	      split -> p_r = p_r;
	    }
	}

      split_node = split_node -> next;
    }

  if ( ! split_list )
    return;

  /* Transaction date */
  date_node = get_child ( transaction_node, "date-posted" );
  date_string = child_content (date_node, "date");
  space = strchr ( date_string, ' ' );
  if ( space )
    *space = 0;
  date = g_date_new ();
  g_date_set_parse ( date, date_string );
  if ( !g_date_valid ( date ))
    fprintf ( stderr, "grisbi: Can't parse date %s\n", date_string );

  /* Tiers */
  tiers = child_content ( transaction_node, "description" );

  /* Create transaction */
  split = split_list -> data;
  transaction = new_transaction_from_split ( split, tiers, date );
  transaction -> operation_ventilee = 0;
  transaction -> ope_de_ventilation = 0;
  account = find_imported_account_by_name ( split -> account );
  if ( account )
    account -> operations_importees = g_slist_append ( account -> operations_importees, transaction );

  /** Breakdowns of transactions are handled the same way, we process
      them if we find more than one split in transaction node. */
  if ( g_slist_length ( split_list ) > 1 )
    {
      transaction -> operation_ventilee = 1;
      transaction -> montant = total;

      while ( split_list )
	{
	  struct gnucash_split * split = split_list -> data;
	  struct struct_compte_importation * account = NULL; 
	  
	  transaction = new_transaction_from_split ( split, tiers, date );
	  transaction -> ope_de_ventilation = 1;

	  account = find_imported_account_by_name ( split -> account );
	  if ( account )
	    account -> operations_importees = g_slist_append ( account -> operations_importees, transaction );
      
	  split_list = split_list -> next;
	}
    }
}



/**
 * Utility functions that returns currency value of a currency node.
 *
 * \param currency_node		XML node to parse.
 *
 * \return string		Representation of currency
 */
gchar * get_currency ( xmlNodePtr currency_node )
{
  return child_content ( currency_node, "id" );
}



/**
 * Find currently imported accounts according to their gnucash uid
 * (guid).
 *
 * \param guid		Textual guid of account to search.
 *
 * \return		A pointer to a struct_compte_importation or NULL upon failure.
 */
struct struct_compte_importation * find_imported_account_by_uid ( gchar * guid )
{
  GSList * liste_tmp;

  if ( ! guid )
    return NULL;

  liste_tmp = liste_comptes_importes;

  while ( liste_tmp )
    {
      struct struct_compte_importation * account = liste_tmp -> data;

      if ( !strcmp ( account -> guid, guid ))
	{
	  return account;
	}

      liste_tmp = liste_tmp -> next;
    }  

  return NULL;
}



/**
 * Find currently imported accounts according to their name.
 *
 * \param guid		Name of account to search.
 *
 * \return		A pointer to a struct_compte_importation or NULL upon failure.
 */
struct struct_compte_importation * find_imported_account_by_name ( gchar * name )
{
  GSList * liste_tmp;

  if ( ! name )
    return NULL;

  liste_tmp = liste_comptes_importes;

  while ( liste_tmp )
    {
      struct struct_compte_importation * account = liste_tmp -> data;

      if ( !strcmp ( account -> nom_de_compte, name ))
	{
	  return account;
	}

      liste_tmp = liste_tmp -> next;
    }  

  return NULL;
}



/**
 * Find currently imported categories according to their gnucash uid
 * (guid).
 *
 * \param guid		Textual guid of category to search.
 *
 * \return		A pointer to a gnucah_category or NULL upon failure.
 */
struct gnucash_category * find_imported_categ_by_uid ( gchar * guid )
{
  GSList * liste_tmp;

  liste_tmp = gnucash_categories;

  while ( liste_tmp )
    {
      struct gnucash_category * categ = liste_tmp -> data;

      if ( !strcmp ( categ -> guid, guid ))
	{
	  return categ;
	}

      liste_tmp = liste_tmp -> next;
    }  

  return NULL;
}



/**
 * Parse a gnucash value representation and construct a numeric
 * representation of it.  Gnucash values are always of the form:
 * "integer/number".  Integer value is to be divided by another
 * integer, which eliminates float approximations.
 *
 * \param value		Number textual representation to parse.
 *
 * \return		Numeric value of argument 'value'.
 */
gdouble gnucash_value ( gchar * value )
{
  gchar **tab_value;
  gdouble number, mantisse;

  tab_value = g_strsplit ( value, "/", 2 );
  
  number = my_atoi ( tab_value[0] );
  mantisse = my_atoi ( tab_value[1] );

  return number / mantisse;
}



/**
 * Manually parse a gnucash file, tidy it, put result in a temporary
 * file and parse it via libxml.
 *
 * \param filename	Filename to parse.
 *
 * \return		A pointer to a xmlDocPtr containing XML representation of file.
 */
xmlDocPtr parse_gnucash_file ( gchar * filename )
{
  gchar buffer[1024], *tempname;
  FILE * filein, * tempfile;
  xmlDocPtr doc;

  filein = utf8_fopen ( filename, "r" );
  if ( ! filein )
  {
      dialogue_error_hint ( g_strdup_printf ( _("Either file \"%s\" does not exist or it is not a regular file."),
					      filename ),
			    g_strdup_printf ( _("Error opening file '%s'." ), filename ) );
      return NULL;
  }

  tempname = g_strdup_printf ( "gsbgnc%05d", g_random_int_range (0,99999) );
  tempfile = utf8_fopen ( tempname, "w+x" );
  if ( ! tempfile )
  {
      dialogue_error_hint ( _("Grisbi needs to open a temporary file in order to import Gnucash data but file can't be created.  Check that you have permission to do that."),
			    g_strdup_printf ( _("Error opening temporary file '%s'." ), tempname ) );
      return NULL;
  }

  /**
   * We need to create a temporary file because Gnucash writes XML
   * files that do not respect the XML specification regarding
   * namespaces.  We need to tidy XML file in order to let libxml
   * handle it gracefully.
   */
  while ( fgets ( buffer, 1024, filein ) )
    {
      gchar * tag;
      tag = g_strrstr ( buffer, "<gnc-v2>" );
      
      if ( tag )
	{
	  gchar * ns[14] = { "gnc", "cd", "book", "act", "trn", "split", "cmdty", 
			     "ts", "slots", "slot", "price", "sx", "fs", NULL };
	  gchar ** iter;

	  tag += 7;
	  *tag = 0;
	  tag++;

	  fputs ( buffer, tempfile );
	  for ( iter = ns ; *iter != NULL ; iter++ )
	    {
	      fputs ( g_strdup_printf ( "  xmlns:%s=\"http://www.gnucash.org/lxr/gnucash/source/src/doc/xml/%s-v1.dtd#%s\"\n", 
					*iter, *iter, *iter ),
		      tempfile );
	    }
	  fputs ( ">\n", tempfile );
	}
      else
	{
	  fputs ( buffer, tempfile );
	}
    }
  fclose ( filein );
  fclose ( tempfile );

  doc = utf8_xmlParseFile ( tempname );

  /** Once parsed, the temporary file is removed as it is useless.  */
  unlink ( tempname );
  
  return doc;
}



/**
 * Find a split in a splits list according to a specific amount and
 * account or category.  This is used to find splits pairs.
 *
 * \param split_list	Split list to lookup.
 * \param amount	Split amount to match against.
 * \param account	Account to match against.
 * \param categ		Category to match against.
 *
 * \return		A gnucash_split upon success.  NULL otherwise.
 */
struct gnucash_split * find_split ( GSList * split_list, gdouble amount, 
				    struct struct_compte_importation * account, 
				    struct gnucash_category * categ )
{
  GSList * tmp;
  
  tmp = split_list;
  while ( tmp )
    {
      struct gnucash_split * split = tmp -> data;
      if ( amount == - split -> amount &&
	   ! ( split -> account && split -> category ) &&
	   ! ( split -> category && categ ) )
	{
	  return split;
	}

      tmp = tmp -> next;
    }
  
  return NULL;
}



/**
 * Update a split with arbitrary informations according to their
 * correctness.  If an account is specified and split already has an
 * account, this means it is the second split of the pair and first
 * split was an account split, so this is transfer split and we set
 * split's contra_account to 'account'.
 * 
 * \param split		Split to update.
 * \param amount	Amount to set if account is not NULL.
 * \param account	Account to set if not NULL.
 * \param categ		Category to set if not NULL.
 */
void update_split ( struct gnucash_split * split, gdouble amount, 
		    gchar * account, gchar * categ )
{
  if ( categ )
    {
      split -> category = g_strdup ( categ );
    }

  if ( account )
    {
      if ( !split -> account )
	{
	  split -> account = g_strdup ( account );
	  split -> amount = amount;
	}
      else
	{
	  split -> contra_account = g_strdup ( account );
	}
    }
}



/**
 * Allocate a new split and set its values to arguments passed.
 *
 * \param amount	Split amount.
 * \param account	Split account.
 * \param categ		Split category.
 *
 * \return		A newly created gnucash_split.
 */
struct gnucash_split * new_split ( gdouble amount, gchar * account, gchar * categ )
{
  struct gnucash_split * split;

  split = calloc ( 1, sizeof ( struct gnucash_split ));

  split -> amount = amount;
  if ( account )
    split -> account = g_strdup ( account );
  else 
    split -> account = NULL;
  if ( categ )
    split -> category = g_strdup ( categ );
  else 
    split -> category = NULL;

  return split;
}



/**
 * Allocate and return a struct_ope_importation created from a
 * gnucash_split and some arguments.
 * 
 * \param split		Split to use as a base.
 * \param tiers		Transaction payee name.
 * \param date		Transaction date.
 *
 * \return 		A newly allocated struct_ope_importation.
 */
struct struct_ope_importation * new_transaction_from_split ( struct gnucash_split * split,
							     gchar * tiers, GDate * date )
{
  struct struct_ope_importation * transaction;

  /** Basic properties are set according to split. */
  transaction = calloc ( 1, sizeof ( struct struct_ope_importation ));
  transaction -> montant = split -> amount;
  transaction -> notes = split -> notes;
  transaction -> p_r = split -> p_r;
  transaction -> tiers = tiers;
  transaction -> date = date;

  if ( split -> contra_account )
    {
      /** If split contains a contra account, then this is a transfer. */
      struct struct_compte_importation * contra_account;
      struct struct_ope_importation * contra_transaction;
	  
      contra_transaction = calloc ( 1, sizeof ( struct struct_ope_importation ));
      contra_transaction -> montant = -split -> amount;
      contra_transaction -> notes = split -> notes;
      contra_transaction -> tiers = tiers;
      contra_transaction -> date = date;

      transaction -> categ = g_strconcat ( _("Transfer"), " : ",
					   split -> contra_account, NULL);
      contra_transaction -> categ = g_strconcat ( _("Transfer"), " : ",
						  split -> account, NULL);

      contra_account = find_imported_account_by_name ( split -> contra_account );
      if ( contra_account )
	contra_account -> operations_importees = g_slist_append ( contra_account -> operations_importees, contra_transaction );
    }
  else
    {
      transaction -> categ = split -> category;
    }

  return transaction;
}

/* Local Variables: */
/* c-basic-offset: 4 */
/* End: */
