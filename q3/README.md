# Report on question 3

## Basic functioning

1. Four semaphores are created, one indicates the number of available acoustic stages, one indicates the number of available electric stages, one indicates c coordinators while the last one indicates the number of stages where musicians are performing and a singer can join.
2. Locks are used for every stage and every performer. They are used appropriately
3. Initially all the k performers are sent to srujana(), and from there on split into various versions and search for stages on which they can perform.
4. The implementation relies on notion of versions, where all possible versions / choices of that performer are released as separate threads, and the one which succeeded (was fast enough) are considered, while the rest are killed.
5. After performances, both singers and musicians collect T-shirts and leave, i.e, their corresponding threads, locks are destroyed. Same goes with semaphores.
6. It is assumed that the name of a performer contains atmost 100 characters.

## Musicians 

1. Initially, a single thread is created for a musician, and is released to srujana().
2. In srujana, the musician arrives at his respective arrival time. After his arrival, his instrument is examined, and the possible stages he can enter (acoustic or electric) are determined.
3. Based on his performance, one or two additional threads are instantiated, in which each act as a version of the musician. For instance if he can perform on both types of stages, two threads are created, one for each type of stage. If that's not the case, only one thread is dispatched.
4. After required threads are created, they are dispatched to the function 'performOnSpecificStage()', where he searches for an empty acoustic / electric stage.
5. A timed wait is applied on the respective stage semaphore, to check if t seconds have passed, which examine if his patience has exhausted.
6. If this thread is the chosen version (other versions didn't get a stage yet and no other version has left impatiently yet), then the execution continues, otherwise, this version (thread) is killed.
7. In this chosen version, the musician searches for required type of stage in a single pass through all stages. After choosing, he updates the status of stage such that he is performing. Also, if he is a non-singer, he signals the singers that they can join him.
8. After this, he starts performing for a random duration which lies between t1 and t2 seconds.
9. After this, he checks if a singer has joined him. If yes, the performance is extended by 2 seconds, and now there are 2 performers on this stage.
10. Performer(s) leave the stage after this point, update the variables of the stage and themselves. Also, if this musician just finished his solo performance, he decreases the number of musicians a singer can join, by 1.
11. After leaving the stage, performer(s) wait for the coordinators to be available. Once the coordinator is available, his status becomes busy, t-shirt(s) is/are collected, and state of the co-ordinator is reupdated. This is handled in getTShirtAlready() function where multiple performers can go at once for collection.
12. Note that, singer and musician both collect their own t-shirts if both performed together. After collecting their t-shirts, they leave the place. 
13. Locks are used to secure critical sections.

## Singers

1. The singers also follow the same execution flow and instructions as musicians, except that we should consider an additional version now.
2. Apart from searching for a free acoustic / electric stage, a singer must also be on the lookout for a stage on which a musician is giving a solo performance, as there is a possibility he can join this stage.
3. This is considered as the third version (thread), which competes with the other two versions (threads) mentioned above. This singer waits for the semaphore which depicts the number of musicians a singer can join this way.
4. If this version succeeds (meaning explained in musicians section), the singer keeps track of the time he has not been assigned a stage, and leaves impatiently if more than t seconds have passed this way.
5. If the above case is not the situation, the singer now looks for a stage in a single pass on all stages (once) on which there is a musician giving solo performance, as there definitely exists one such stage, because the semaphore allowed him to search.
6. On finding such a stage, he updates the variables of the stage, himself, and his partner musician.
7. Now, after the updations, he just returns from this version, because the part where he explicitly joins the musician and collects his t-shirt is handled in the version(s) dispatched to performOnSpecificStage().
8. Locks are used appropriately.


## Coordinators

1. A semaphore is used to keep track of all the coordinators who are available at a moment. This part is handled in getTShirtAlready().
2. Whenever a perfomer leaves the stage after his performance, he waits for a coordinator to attend him and give him his t-shirt.
3. Coordinator can attend to only one performer at once, and takes 2 seconds to find and give him the t-shirt of respective size.
4. Once a free coordinator is about to attend to a performer, his status is updated to busy (wait in semaphore) and once the performer is serviced, it is signalled that this coordinator is free.

## Stages

1. There is a `stage_is_free` array which can take three states - stage is free, a singer can join this stage (musician performing solo), a singer cannot join this stage (a singer is performing already). These values for the corresponding stage are updated wherever necessary (typically after performances / when a musician or singer joins the stage).
2. Every stage is reset after a performance finishes on this.
3. Every stage also keeps track of who is performing currently on it, with the help of `stage_performer` and `singer_partner` arrays.
4. It is also assumed that the stages are numbered such that, in the total of (a + e) stages, first 'a' stages are acoustic and numbered from 1 to 'a' (inclusive), and the next 'e' stages are electric and are numbered from 'a+1' to 'a+e' (inclusive).

## Bonus

1. Singers collecting t-shirts is included in the execution flow, as explained above.
2. The array `stage_num` is used to keep track about the stage number on which the performer (musician / singer) is performing on.
