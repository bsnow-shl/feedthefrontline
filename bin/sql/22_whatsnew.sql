
CREATE TABLE whatsnew (
    whatsnew_id integer NOT NULL PRIMARY KEY,

	-- name, link, description of the RSS feed
    title text NOT NULL,
    short_name text,
    uri text NOT NULL,
    description text NOT NULL,

	-- whatsnew is fed three ways:
	-- 1. a callback function (lame)
	callback_function text,
	callback_header text,


	-- 2. autogenerated from a table (cool)
	-- defined by a table name
    source_table text,

	-- that table's pkey
    source_table_pkey text,

	-- a where clause (and ORDER BY) to return items of interest
    where_clause text,

	-- and the column names where the title and caption and date come from
    title_key text,
    description_key text,
    date_key text,

	-- this callback function to generate link URI's
    link_header text,
    link_function text,

	-- 3. autogenerated from a i18n'ized table (cooler)
	-- just define these two and title and description key will be read
	-- as referring to the i18n data
    i18n_class text,
    i18n_language character(2)
);



INSERT INTO whatsnew (whatsnew_id, title, uri, description, source_table, source_table_pkey, where_clause, i18n_class, i18n_language, title_key, description_key, link_header, link_function, date_key) VALUES (1, 'New users', '/admin/users/', 'Recent users', 'users', 'user_id', 'where true order by member_since desc', NULL, NULL, ' first_name ||'' '' || last_name', NULL, 'users.h', 'user_whatsnew_callback', 'member_since');


