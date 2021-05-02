# Report on question 2

## Basic functioning

1. Required number of student threads, zone threads and company threads are created in the main functions which are then dispatched to their corresponding functions, and are joined at the end. 
2. These 3 independent dispatch functions communicate with each other by the means of global variables which they share. 
3. Locks are used appropriately.

## Pharmaceutical companies

1. All the n company threads created execute the function `company_dispatch` which takes company ID as its argument. This function consists of an infinite loop, in which, the company just waits if its current number of batches aren't zero, and begins production otherwise. 
2. It maintains arrays to maintain count of vaccines of each batch it currently possesses, and the number of batches it currently possesses. 
3. It uses the function produceVaccines() for production and terminates when all students are processed.
4. Mutex locks are used when global variables are accessed.

## Vaccination zones

1. All the m vaccination zones created execute the function `zone_dispatch` which takes the zone ID as an argument. It functions as long as there are students to be processed. 
2. It first iterates through all companies, and selects a company which has atleast one batch of vaccines. Thus, this zone takes those vaccines and updates its vaccines count and the company's vaccines its utilizing. 
3. Now, it has some vaccines. It now waits to check if atleast one student is waiting for a zone's slot. Otherwise, the waiting students is 0, which is not ideal for randomization, and the slots declaration would be wasted.
4.  If above condition is satisfied, valid number of slots are declared (not allotted to students yet).
5. Now, it waits until it has atleast one slot available and atleast one student is waiting, before it enters vaccination phase. This is allocation of slots to students. This is done because the zone doesn't want a student to be waiting if it has an empty slot.
6. Now the count of slots allocated to students (not necessarily the same as number of slots declared), is stored, and it eventually, busy waits, till all the students whose slots got allocated here are vaccinated (in `student_dispatch` function). This is the vaccination phase.
7. This process is repeated until all students are processed.
8. It maintains global arrays for all the above purposes. Helper functions getSlots() and updateSlots() are used. Mutex locks are used appropriately.

## Students

1. All the o students created execute the function `stud_dispatch` which takes the student ID as an argument. It functions until all students are processed. 
2. Student entry time is randomized. After entering the current attempt, student waits for a slot to be allocated and increments the number of waiting students by 1.
3. For the particular student, his attempt count is updated and checked before processing, and if that value exceeds 3, it means he faced 3 unsuccessful vaccinations, and hence, he is sent home.
His success status is updated in succ array, which contains student's vaccination success status.
4. If that is not the case, the student iterates through all the zones that are ready to be allocated. He tries to find an empty slot in a zone which is not in vaccination phase currently (as it wouldn't be allocating slots in that case). He updates the corresponding slot count and vaccine count of the zone he chooses. He also decrements waiting students count, as his waiting phase is finished now.
5. Student waits for the chosen zone to enter vaccination phase, and gets vaccinated. The function vaccSuccessful() is used to determine if the vaccine was successful.
6. If the vaccine was successful, the student can go to college. Hence, he returns from this function, after updating the succ array (this array is useful to check if all students are processed).
7. If it failed (negative for anti-bodies), then he goes through above procedure again, which acts as the next attempt of the student.
8. Mutex locks are used while accessing global variables (in critical sections). Global arrays are used for above purposes. 
