-- The i18n system identifies a series of classes each with an ordered list of fields.
-- e.g. a press release class with fields "headline", "byline", "body", and "photo"
-- these are in i18n_classes and i18n_fields
--
-- it also identifies a set of languages that are active (i.e. can be published) on a site
-- these are in i18n_languages
-- 
-- objects of these classes are stored in the 18n table, with one row per language per field.
-- i.e. if you have a press release in 2 languages, that'll be 8 rows in i18n (2 lang x 4 fields)
--

create table i18n_languages (
    language_key char(2) not null primary key,
    language_name text,

    precedence integer default 10,
    active integer default 0 check(active in (0,1))
);
insert into i18n_languages (language_key, language_name, active)
    values ('en', 'English', 1);


create table i18n_classes (
	class_id integer not null primary key,

	-- for presentation
	class_name varchar(50) not null unique,

	pretty_name_en text,
	pretty_name_fr text,

	-- storage preferences
		-- 1=i18n 2=table, 3=fileysstem
		read_medium integer not null default 1,
		check (read_medium  in  (1,2,3)),

		store_i18n integer default 0,
		store_table integer default 0,
		store_filesystem integer default 0,

		table_name varchar(32), -- specifically NOT UNIQUE

	check ( not (
		table_name is null and (store_table!=0 or read_medium=2)
	)),

	editable_in_cms integer not null default 0,
	check (editable_in_cms in (0,1) and
		store_table in (0,1) and
		store_i18n in (0,1) and
		store_filesystem in (0,1)
	)
);

create table i18n_fields (
	field_id integer not null primary key,
	class_id integer not null references i18n_classes on delete cascade,
	precedence integer default 100,

	field_name text not null,
	field_type text not null,
	mandatory               integer not null default 0,
    multiple                integer not null default 0,

	language_independent	integer not null default 0,
	check (language_independent in (1,0)),

	-- one for each language I guess...
	pretty_name_en text not null,
	entry_explanation_en    text,
	help_text_en			text,

	pretty_name_fr text,
	entry_explanation_fr text,
	help_text_fr text,
	

	-- stuff to drive things in the database
	column_name             varchar(30),
	pgsql_data_type         varchar(30),   -- "varchar(4000)"
	pgsql_extra             text,
	
	include_in_fulltext_index integer not null default 0,
	check (include_in_fulltext_index in (0,1)),

	-- presentation matters
	presentation_options    text,
    display_rows            integer not null default 0,
    display_columns         integer not null default 0,

	check (mandatory in (1,0)),
	default_value           varchar(200),

	unique(class_id, field_name),
	unique (class_id, column_name)
);
create index i18n_field_class on i18n_fields (class_id);


create table i18n (
    field_id integer not null references i18n_fields  on delete cascade,

    object_id integer not null,
    language_key char(2) references i18n_languages  on delete cascade,
    content text,

    unique(field_id,object_id,language_key)
);



-- every piece of content can list its access policy
	create table i18n_access_rights (
	    access_right_id integer not null primary key,
	    name text not null,
	    implementation_function text ,
	    display_order integer not null);

	insert into i18n_access_rights values ('1','public',null, 1);
	insert into i18n_access_rights values ('2','account required','content_user_require',2);



-- official list of associations
create table i18n_association_types (
	association_type_id integer not null primary key,
	pretty_name text not null,
	class_a		integer not null references i18n_classes(class_id)  on delete cascade,
    class_b		integer not null references i18n_classes(class_id)  on delete cascade,
	cardinality text not null,
	check (cardinality in ('1-1', '1-m', 'm-1', 'm-m'))
);

create table i18n_associations (
	association_type_id integer references i18n_association_types  on delete cascade,
	
    object_id_a		integer not null,
    object_id_b		integer not null,

	-- the objects are uniquely identified above but let's save ourselves
	-- hassle by recording in which tables to find them
	class_a		integer not null references i18n_classes(class_id) on delete cascade,
    class_b		integer not null references i18n_classes(class_id) on delete cascade,

        -- User-entered reason for relating two objects, e.g.
        -- to distinguish between John McCarthy the developer of
		-- Lisp and Gerry Sussman and Guy Steele, who added lexical scoping
        -- in the Scheme dialect 
	map_comment		varchar(4000),
	
	creation_user		integer not null references users  on delete cascade,
	creation_tstamp		timestamp not null default CURRENT_TIMESTAMP,
    primary key (object_id_a, object_id_b)
);


-- the above can be used to create a multilingual
-- reference for CPF objects.  For instance, if you have a 
-- website that does announcements in en,fr,es languages, you can
-- look up "Announcement" -> pkey and get the three translations





-- now, slather a content management system on top of i18n

create table i18n_content (
 	-- a content item
	content_id integer not null primary key,

	-- is an instance of a particular class
	class_id integer not null references i18n_classes  on delete cascade,

	-- might be locked so only superusers can delete it
	locked integer default 0,
	check(locked in (0,1)),


	-- typically this is the URL it can be found at, i.e. /content/foo
	short_name text,
	
	-- read
	access_right_id integer references i18n_access_rights(access_right_id) ,

	-- write
	write_access_right_id integer references i18n_access_rights(access_right_id),

	-- update triggers
	on_update_header text,
	on_update_function text,

	-- administrivia
	create_tstamp timestamp default CURRENT_TIMESTAMP
);
create index content_short_name on i18n_content  (short_name);


create table i18n_content_hierarchy (
  parent_content_id integer references i18n_content  on delete cascade,
  display_order integer not null,

  content_id integer references i18n_content
);
create index content_hier_parent on i18n_content_hierarchy (parent_content_id);
create index content_hier_child on i18n_content_hierarchy (content_id);



-- then which object ID is presently live at those URI's
create table i18n_content_map (
	content_id integer not null references i18n_content,

	-- null means never
	posting_datetime timestamp  default CURRENT_TIMESTAMP,

	-- null means forever
	takedown_datetime timestamp,

	-- which one is live now
	object_id integer,
	
	create_datetime timestamp not null default CURRENT_TIMESTAMP,
	created_by integer references users(user_id)	
);


insert into i18n_classes(class_id,class_name) values (140,'Content category');
insert into i18n_fields(field_id, class_id,field_name,pretty_name_en,field_type) values (14, 140, 'title', 'Title', 'text');

insert into i18n_classes(class_id,class_name) values (141,'Content item');
insert into i18n_fields(field_id, class_id,field_name,pretty_name_en,field_type) values (15, 141, 'title', 'Title', 'text');

insert into roles (role_name) values ('content_admin');
insert into roles (role_name) values ('content_nerd');
insert into roles (role_name) values ('content_type_admin');
insert into roles (role_name) values ('content_super_admin');

insert into admin_areas(uri,description,role_id) 
        select '/admin/content/', 'Content Management', role_id from roles where role_name='content_admin';





-- this is used for editing the gettext dictionary
	create table temp_translations(
	    translation_id serial, 
	    source text not null, 
	    dest text not null, 
	    needs_translation integer not null default 0
	);
	create index temp_translation_needed on temp_translations(needs_translation);





-- language translations in i18n itself
	create table i18n_translation_types (
		translation_type_id integer not null primary key,
		from_language char(2) not null references i18n_languages(language_key),
		to_language char(2) not null references i18n_languages(language_key)
	);

	-- list of pending translation jobs, so if say English is updated, the French can be marked in need of fixing
	create table i18n_txn_todo (
		translation_type_id integer not null references i18n_translation_types,
		field_id integer not null references i18n_fields,
		object_id integer not null 
	);
	create index i18n_txn_jobs on i18n_txn_todo(field_id, object_id);
