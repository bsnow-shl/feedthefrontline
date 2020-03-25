
create table document_categories (
	document_category_id integer not null primary key,

	-- null means don't display it
	display_order integer default 100,
	
	-- permissions configuration
	allow_comments integer not null check (allow_comments in (0,1)),
	admin_only integer not null check (allow_comments in (0,1)),
	restrict_to_role text,
	restrict_to_special text
);
alter table document_categories add column parent_category_id integer references document_categories(document_category_id);


insert into i18n_classes (class_id,class_name) values (1,'document category');
insert into i18n_fields (field_id,class_id,field_name,pretty_name_en,field_type) values (101,1, 'name', 'Category Name','text');
insert into i18n_fields (field_id,class_id,field_name,pretty_name_en,field_type) values (102,1, 'description', 'Category Description', 'bigtext');


create table documents (
	document_id integer not null primary key,
	document_category_id integer not null references document_categories,

	order_in_category integer not null default 100,

	upload_id integer references uploads,

	posting_date date default CURRENT_DATE,
	takedown_date date ,

	-- if it's an image
	image_width integer,
	image_height integer
);

insert into i18n_classes (class_id,class_name) values (2,'document');
insert into i18n_fields (field_id,class_id,field_name,pretty_name_en,field_type) values (201,2, 'name', 'Name','text');
insert into i18n_fields (field_id,class_id,field_name,pretty_name_en,field_type) values (202,2, 'description', 'Description','bigtext');


-- publish it into admin menus
insert into roles (role_name) values ('document_admin');
insert into roles (role_name) values ('document_category_admin');

insert into admin_areas(uri,description,role_id) 
	select '/admin/documents/', 'Document Repository', role_id from roles where role_name='document_admin';


insert into comment_systems (comment_system_id, singular, plural, table_name, pkey_column, title_column) values
	(3, 'document', 'documents', 'documents', 'document_id', 'pretty_name');


