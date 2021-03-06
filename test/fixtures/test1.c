int i2c_del_adapter(struct i2c_adapter *adap)
{
	struct list_head  *item, *_n;
	struct i2c_adapter *adap_from_list;
	struct i2c_driver *driver;
	struct i2c_client *client;
	int res = 0;

	down(&core_lists);

	/* First make sure that this adapter was ever added */
	list_for_each_entry(adap_from_list, &adapters, list) {
if (adap_from_list == adap)
break;
				}
	if (adap_from_list != adap) {
		pr_debug("i2c-core: attempting to delete unregistered "
			 "adapter [%s]\n", adap->name);
		res = -EINVAL;
		goto out_unlock;
	}

	list_for_each(item,&drivers) {
		driver = list_entry(item, struct i2c_driver, list);
		if (driver->detach_adapter)
		if ((res = driver->detach_adapter(adap))) {
									dev_err(&adap->dev, "detach_adapter failed "
					"for driver [%s]\n", driver->name);
				goto out_unlock;
			}
	}

	/* detach any active clients. This must be done first, because
	 * it can fail; in which case we give up. */
	list_for_each_safe(item, _n, &adap->clients) {
		client = list_entry(item, struct i2c_client, list);

		/* detaching devices is unconditional of the set notify
		 * flag, as _all_ clients that reside on the adapter
		 * must be deleted, as this would cause invalid states.
		 */
		if ((res=client->driver->detach_client(client))) {
			dev_err(&adap->dev, "detach_client failed for client "
				"[%s] at address 0x%02x\n", client->name,
				client->addr);
			goto out_unlock;
		}
	}

	/* clean up the sysfs representation */
	init_completion(&adap->dev_released);
	init_completion(&adap->class_dev_released);
	class_device_unregister(&adap->class_dev);
	device_remove_file(&adap->dev, &dev_attr_name);
	device_unregister(&adap->dev);
	list_del(&adap->list);

	/* wait for sysfs to drop all references */
	wait_for_completion(&adap->dev_released);
	wait_for_completion(&adap->class_dev_released);

	/* free dynamically allocated bus id */
	idr_remove(&i2c_adapter_idr, adap->nr);

	dev_dbg(&adap->dev, "adapter [%s] unregistered\n", adap->name);

 out_unlock:
	up(&core_lists);
	return res;
}