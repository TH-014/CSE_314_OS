#include <bits/stdc++.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
using namespace std;

#define _clock chrono::high_resolution_clock::now()
#define nano_t chrono::duration_cast<std::chrono::nanoseconds>
#define micro_t chrono::duration_cast<std::chrono::microseconds>
#define milli_t chrono::duration_cast<std::chrono::milliseconds>

#define STANDARD_MIN_ID 1001
#define STANDARD_MAX_ID 1100
#define PREMIUM_MIN_ID 2001
#define PREMIUM_MAX_ID 2100

#define GALLERY1_CAPACITY 5
#define CORRIDOR_CAPACITY 3
#define DELAY_MULTIPLIER 1000
#define STEPS_DELAY 1000
#define MAX_WALKING_DELAY 10000
#define MIN_WALKING_DELAY 5000

sem_t gallery1_semaphore, corridor_semaphore;
pthread_mutex_t step_mutex_1, step_mutex_2, step_mutex_3, photo_booth_mutex;
pthread_mutex_t pb_access_mutex, shared_pb_mutex, exclusive_pb_mutex;

int num_of_std_vis, num_of_prm_vis, hallway_ab_delay, gallery1_delay, gallery2_delay, photo_booth_delay;
int premium_cnt = 0, standard_cnt = 0;

class Visitor;
void write_output(Visitor &visitor);

int get_random_number()
{
    // Creates a random device for non-deterministic random number generation
    std::random_device rd;
    // Initializes a random number generator using the random device
    std::mt19937 generator(rd());

    // Lambda value for the Poisson distribution
    double lambda = 5014.14;

    // Defines a Poisson distribution with the given lambda
    std::poisson_distribution<int> poissonDist(lambda);

    // Generates and returns a random number based on the Poisson distribution
    return poissonDist(generator);
}

auto tbegin = _clock;

long long get_time()
{
    auto tend = _clock;
    long long elapsed_time_ms = milli_t(tend - tbegin).count();
    return elapsed_time_ms;
}

enum visitor_state
{
    ARRIVED_A,
    ARRIVED_B,
    AT_STEP_1,
    AT_STEP_2,
    AT_STEP_3,
    ARRIVED_C,
    ARRIVED_D,
    ARRIVED_E,
    WAITING_PB,
    AT_PHOTO_BOOTH,
    ARRIVED_F
};

class Visitor
{
public:
    int id;
    enum visitor_state state;
    Visitor(int id) : id(id) {}
    bool is_premium()
    {
        return id >= PREMIUM_MIN_ID && id <= PREMIUM_MAX_ID;
    }
    void set_state(enum visitor_state state)
    {
        this->state = state;
        write_output(*this);
    }
};

pthread_mutex_t output_lock;

void write_output(Visitor &visitor)
{
    pthread_mutex_lock(&output_lock);
    cout << "Visitor " << visitor.id;
    switch (visitor.state)
    {
    case ARRIVED_A:
        cout << " has arrived at A";
        break;
    case ARRIVED_B:
        cout << " has arrived at B";
        break;
    case AT_STEP_1:
        cout << " is at step 1";
        break;
    case AT_STEP_2:
        cout << " is at step 2";
        break;
    case AT_STEP_3:
        cout << " is at step 3";
        break;
    case ARRIVED_C:
        cout << " is at C (entered Gallery 1)";
        break;
    case ARRIVED_D:
        cout << " is at D (exiting Gallery 1)";
        break;
    case ARRIVED_E:
        cout << " is at E (entered Gallery 2)";
        break;
    case WAITING_PB:
        cout << " is about to enter the photo booth";
        break;
    case AT_PHOTO_BOOTH:
        cout << " is inside the photo booth";
        break;
    case ARRIVED_F:
        cout << " is at F (exiting Gallery 2)";
        break;
    }
    cout << " at timestamp " << get_time() << endl;
    pthread_mutex_unlock(&output_lock);
}

void gain_shared_access(Visitor *visitor)
{
    pthread_mutex_lock(&pb_access_mutex);
    pthread_mutex_lock(&shared_pb_mutex);
    standard_cnt++;
    if (standard_cnt == 1)
        pthread_mutex_lock(&photo_booth_mutex);
    pthread_mutex_unlock(&shared_pb_mutex);
    pthread_mutex_unlock(&pb_access_mutex);

    visitor->set_state(AT_PHOTO_BOOTH);
    usleep(photo_booth_delay * DELAY_MULTIPLIER);

    pthread_mutex_lock(&shared_pb_mutex);
    standard_cnt--;
    if (standard_cnt == 0)
        pthread_mutex_unlock(&photo_booth_mutex);
    pthread_mutex_unlock(&shared_pb_mutex);
}

void gain_exclusive_access(Visitor *visitor)
{
    pthread_mutex_lock(&exclusive_pb_mutex);
    premium_cnt++;
    if (premium_cnt == 1)
        pthread_mutex_lock(&pb_access_mutex);
    pthread_mutex_unlock(&exclusive_pb_mutex);
    pthread_mutex_lock(&photo_booth_mutex);

    visitor->set_state(AT_PHOTO_BOOTH);
    usleep(photo_booth_delay * DELAY_MULTIPLIER);

    pthread_mutex_unlock(&photo_booth_mutex);
    pthread_mutex_lock(&exclusive_pb_mutex);
    premium_cnt--;
    if (premium_cnt == 0)
        pthread_mutex_unlock(&pb_access_mutex);
    pthread_mutex_unlock(&exclusive_pb_mutex);
}

void *visitor_activities(void *arg)
{
    Visitor *visitor = (Visitor *)arg;
    int random_delay = get_random_number() % (num_of_std_vis + num_of_prm_vis) + 1;
    usleep(random_delay * DELAY_MULTIPLIER);
    visitor->set_state(ARRIVED_A);
    usleep(hallway_ab_delay * DELAY_MULTIPLIER);
    visitor->set_state(ARRIVED_B);
    // usleep(STEPS_DELAY);
    pthread_mutex_lock(&step_mutex_1);
    visitor->set_state(AT_STEP_1);
    usleep(STEPS_DELAY);
    pthread_mutex_lock(&step_mutex_2);
    pthread_mutex_unlock(&step_mutex_1);
    visitor->set_state(AT_STEP_2);
    usleep(STEPS_DELAY);
    pthread_mutex_lock(&step_mutex_3);
    pthread_mutex_unlock(&step_mutex_2);
    visitor->set_state(AT_STEP_3);
    usleep(STEPS_DELAY);
    sem_wait(&gallery1_semaphore);
    pthread_mutex_unlock(&step_mutex_3);
    visitor->set_state(ARRIVED_C);
    usleep(gallery1_delay * DELAY_MULTIPLIER);
    sem_wait(&corridor_semaphore);
    sem_post(&gallery1_semaphore);
    visitor->set_state(ARRIVED_D);
    usleep(get_random_number() % (MAX_WALKING_DELAY - MIN_WALKING_DELAY + 1) + MIN_WALKING_DELAY);
    visitor->set_state(ARRIVED_E);
    sem_post(&corridor_semaphore);
    usleep(gallery2_delay * DELAY_MULTIPLIER);
    visitor->set_state(WAITING_PB);
    if (visitor->is_premium())
        gain_exclusive_access(visitor);
    else
        gain_shared_access(visitor);
    visitor->set_state(ARRIVED_F);
    return nullptr;
}

void init()
{
    sem_init(&gallery1_semaphore, 0, GALLERY1_CAPACITY);
    sem_init(&corridor_semaphore, 0, CORRIDOR_CAPACITY);
    pthread_mutex_init(&step_mutex_1, NULL);
    pthread_mutex_init(&step_mutex_2, NULL);
    pthread_mutex_init(&step_mutex_3, NULL);
    pthread_mutex_init(&photo_booth_mutex, NULL);
    pthread_mutex_init(&pb_access_mutex, NULL);
    pthread_mutex_init(&shared_pb_mutex, NULL);
    pthread_mutex_init(&exclusive_pb_mutex, NULL);
}

int main(int argc, char *argv[])
{
    if (argc != 7)
    {
        cerr << "Usage: ./a.out N M w x y z" << endl;
        return 1;
    }
    num_of_std_vis = atoi(argv[1]);
    num_of_prm_vis = atoi(argv[2]);
    hallway_ab_delay = atoi(argv[3]);
    gallery1_delay = atoi(argv[4]);
    gallery2_delay = atoi(argv[5]);
    photo_booth_delay = atoi(argv[6]);

    init();

    pthread_t threads[num_of_std_vis + num_of_prm_vis];

    for (int i = 0; i < num_of_std_vis; i++)
    {
        Visitor *v = new Visitor(STANDARD_MIN_ID + i);
        pthread_create(&threads[i], NULL, visitor_activities, v);
    }

    for (int i = 0; i < num_of_prm_vis; i++)
    {
        Visitor *v = new Visitor(PREMIUM_MIN_ID + i);
        pthread_create(&threads[num_of_std_vis + i], NULL, visitor_activities, v);
    }

    for (int i = 0; i < num_of_std_vis + num_of_prm_vis; i++)
    {
        // cout << "Joining thread " << i << endl;
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&gallery1_semaphore);
    sem_destroy(&corridor_semaphore);
    pthread_mutex_destroy(&step_mutex_1);
    pthread_mutex_destroy(&step_mutex_2);
    pthread_mutex_destroy(&step_mutex_3);
    pthread_mutex_destroy(&photo_booth_mutex);
    pthread_mutex_destroy(&pb_access_mutex);
    pthread_mutex_destroy(&shared_pb_mutex);
    pthread_mutex_destroy(&exclusive_pb_mutex);

    return 0;
}