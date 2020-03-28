

CREATE TABLE announcement_categories (
    announcement_category_id integer NOT NULL,
    rss_uri text
);

create table announcements (
    announcement_id integer not null primary key,

    -- foreign system reference
    foreign_system text,		-- e.g. "events" for event registrations
    foreign_system_pkey integer,	-- e.g. "25" for event #25

		-- category -- see config.h/announcements
		category integer,

    -- replacements for the body
    url text,
    upload_id integer references uploads,

    start_date date not null default CURRENT_TIMESTAMP,
    end_date date,

    posting_date timestamp not null default CURRENT_TIMESTAMP,

    -- null means "after start date"
    takedown_date timestamp

);

insert into i18n_classes (class_id,class_name) values (3,'announcement');
insert into i18n_fields (field_id,class_id,field_name,pretty_name_en,field_type) values (301,3, 'title', 'Title','text');
insert into i18n_fields (field_id,class_id,field_name,pretty_name_en,field_type) values (302, 3, 'caption', 'Caption', 'text');
insert into i18n_fields (field_id,class_id,field_name,pretty_name_en,field_type) values (303, 3, 'body', 'Body', 'bigtext');


insert into roles (role_name) values ('announcement_admin');

insert into admin_areas(uri,description,role_id) 
	select '/admin/ann/', 'Announcements', role_id from roles where role_name='announcement_admin';
