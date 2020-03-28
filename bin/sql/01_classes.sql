create table classes (
    class_id integer not null primary key ,

    class_name text not null unique,

    table_name text not null,
    table_pkey_column text not null,
    table_pretty_column text not null
);

create table pkeys(
	table_name text,
	column_name text,
	value integer
);
create index pkeys_column_name on pkeys(column_name);

    
