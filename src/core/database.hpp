#ifndef NOTHING_DATABASE_HPP
#define NOTHING_DATABASE_HPP

class Database
{
	public:
		Database() = default;
		~Database() = default;

		Database(const Database &) = delete;
		Database(Database &&) = delete;

		Database &operator =(const Database &) = delete;
		Database &operator =(Database &&) = delete;
};

#endif
