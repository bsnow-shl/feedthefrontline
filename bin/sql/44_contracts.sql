

--T: add a contract
--T: change contract due days
--T: change contract terms
--T: retire a contract
--T: unretire a contract
create table contracts (
	contract_id integer not null primary key,

	retired integer default 0,
	check (retired in (0,1)),

	short_name text,

	party_1 integer not null references users,
	party_2 integer not null references users,

	due_days integer not null default 30,

	contract_terms text
);


--T: create an invoice up to today
--T: create an invoice for items in a date range
--T: uncreate an invoice
create table contract_invoices (
	invoice_id integer not null primary key,
	
	contract_id integer not null references contracts,
	cached_amount integer not null,
	create_date timestamp default CURRENT_TIMESTAMP,
	due_date date,

	created_by integer references users,

	void_reason text,
	void integer not null default 0,
	check (void in (0,1)),

	paid integer not null default 0,
	check (paid in (0,1))
);
create index contract_invoices_contract_idx on contract_invoices(contract_id);


--T: record a payment
--T: record a payment, marking the invoice paid
--T: record a comment
create table contract_invoice_diary (
	invoice_id integer references contract_invoices,
	tstamp timestamp default CURRENT_TIMESTAMP,

	-- payment_amounts should be the reverse of invoice item amounts, i.e. two invoice items for +5.50 should be balanced by a payment of -11.00.
	payment_amount integer,
	note text
);
create index contract_invoice_diary_contract_idx on contract_invoice_diary(invoice_id);




--T: add an invoice item
--T: void it
--T: anti-transaction: change it (update trigger?)
create table contract_invoice_items (
	invoice_item_id integer not null primary key,

	-- an item is invoiced on a contract
	contract_id integer not null references contracts,
	create_date timestamp default CURRENT_TIMESTAMP,
	created_by integer references users,

	-- null means not invoiced yet
	invoice_id integer references contract_invoices,


	-- the basics: how much, when, for what

	-- positive: owed to party_1
	-- negative: owed to party_2
	amount integer not null,
	name text,
	
	-- it might have been withdrawn
	void integer not null default 0,
	check (void in (0,1)),

	void_reason text,

	-- is it tax?
	is_tax integer not null default 0,
	check (is_tax in (0,1)),

	
	-- e.g. 'shopcart' and '99'
	referring_system text,
	referring_system_pkey integer
);
create index contract_item_contract_idx on contract_invoice_items(contract_id);


--T: add an invoice item diary entry
create table contract_invoice_item_diary (
	invoice_item_id integer references contract_invoice_items,
	tstamp timestamp default CURRENT_TIMESTAMP,
	note text
);
create index contract_item_diary_item_idx on contract_invoice_item_diary(invoice_item_id);

	
insert into roles (role_name) values ('contracts_admin');
insert into admin_areas(uri,role_id,description)
        select '/admin/contracts/',  role_id, 'Contracts and invoices' from roles where role_name='contracts_admin';


