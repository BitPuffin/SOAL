typedef size_t type_id;
type_id __type_id_counter;

type_id gentypeid()
{
	return __type_id_counter++;
}

struct typespec {
	type_id id;
	char *name;
	size_t size;
};

enum builtin_type {
	TYPE_INT
};
struct typespec *typespecs;

struct typespec mktypespec(char *name, size_t size)
{
	struct typespec ts = {
		.id = gentypeid(),
		.name = name,
		.size = size,
	};
	return ts;
}

void types_init()
{
	arrput(typespecs, mktypespec("int", sizeof(i64)));
}
