
CREATE TABLE faqs (
	faq_id integer NOT NULL primary key,
	faq_name text NOT NULL,
	short_name text
);

CREATE TABLE faq_questions (
	faq_question_id integer NOT NULL primary key,
	faq_id integer NOT NULL references faqs,
	order_in_faq integer
);

insert into roles (role_name) values ('faq_admin');
insert into admin_areas(uri,description,role_id) 
    select '/admin/faq/', 'FAQ', role_id from roles where role_name='faq_admin';

